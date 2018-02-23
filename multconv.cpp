// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"

#include "mswd6_32.h"
#include "multconv.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef CONVERTERS
   AConverter* AConverter::m_pThis = NULL;
#endif

#define BUFFSIZE 4096

ATrackFile::ATrackFile(CMDIFrameWnd* pWnd) : CFile()
{
	m_nLastPercent = -1;
	m_dwLength = 0;
	m_pFrameWnd = pWnd;
	VERIFY(m_strComplete.LoadString(IDS_COMPLETE));
	VERIFY(m_strWait.LoadString(IDS_PLEASE_WAIT));
	VERIFY(m_strSaving.LoadString(IDS_SAVING));
}

ATrackFile::~ATrackFile()
{
	OutputPercent(100);
	if (m_pFrameWnd != NULL)
		m_pFrameWnd->SetMessageText(AFX_IDS_IDLEMESSAGE);
}

UINT ATrackFile::Read(void FAR* lpBuf, UINT nCount)
{
	UINT n = CFile::Read(lpBuf, nCount);
	if (m_dwLength != 0)
		OutputPercent((int)((GetPosition()*100)/m_dwLength));
	return n;
}

void ATrackFile::Write(const void FAR* lpBuf, UINT nCount)
{
	CFile::Write(lpBuf, nCount);
	OutputString(m_strSaving);
}

void ATrackFile::OutputString(LPCTSTR lpsz)
{
	if (m_pFrameWnd != NULL)
	{
		m_pFrameWnd->SetMessageText(lpsz);
		CWnd* pBarWnd = m_pFrameWnd->GetMessageBar();
		if (pBarWnd != NULL)
			pBarWnd->UpdateWindow();
	}
}

void ATrackFile::OutputPercent(int nPercentComplete)
{
	if (m_pFrameWnd != NULL && m_nLastPercent != nPercentComplete)
	{
		m_nLastPercent = nPercentComplete;
		TCHAR buf[64];
		int n = nPercentComplete;
		wsprintf(buf, (n==100) ? m_strWait : m_strComplete, n);
		OutputString(buf);
	}
}

UINT AOemFile::Read(void FAR* lpBuf, UINT nCount)
{
	UINT n = ATrackFile::Read(lpBuf, nCount);
	OemToCharBuffA((const char*)lpBuf, (char*)lpBuf, n);
	return n;
}

void AOemFile::Write(const void FAR* lpBuf, UINT nCount)
{
	CharToOemBuffA((const char*)lpBuf, (char*)lpBuf, nCount);
	ATrackFile::Write(lpBuf, nCount);
}

#ifdef CONVERTERS

HGLOBAL AConverter::StringToHGLOBAL(LPCSTR pstr)
{
	HGLOBAL hMem = NULL;
	if (pstr != NULL)
	{
		hMem = GlobalAlloc(GHND, (lstrlenA(pstr)*2)+1);
		char* p = (char*) GlobalLock(hMem);
		ASSERT(p != NULL);
		if (p != NULL)
			lstrcpyA(p, pstr);
		GlobalUnlock(hMem);
	}
	return hMem;
}

AConverter::AConverter(LPCTSTR pszLibName, CMDIFrameWnd* pWnd) : ATrackFile(pWnd)
{
	USES_CONVERSION;
	m_hBuff = NULL;
	m_pBuf = NULL;
	m_nBytesAvail = 0;
	m_nBytesWritten = 0;
	m_nPercent = 0;
	m_hEventFile = NULL;
	m_hEventConv = NULL;
	m_bDone = TRUE;
	m_bConvErr = FALSE;
	m_hFileName = NULL;

   m_hLibCnv = LoadLibrary(pszLibName);
	if (m_hLibCnv < (HINSTANCE)HINSTANCE_ERROR)
		m_hLibCnv = NULL;
	else
	{
		LoadFunctions();
		ASSERT(m_pInitConverter != NULL);
		if (m_pInitConverter != NULL)
		{
			CString str = AfxGetAppName();
			str.MakeUpper();
			VERIFY(m_pInitConverter(AfxGetMainWnd()->GetSafeHwnd(), T2CA(str)));
		}
	}
}

AConverter::AConverter(CMDIFrameWnd* pWnd) : ATrackFile(pWnd)
{
	m_pInitConverter = NULL;
	m_pIsFormatCorrect = NULL;
	m_pForeignToRtf = NULL;
	m_pRtfToForeign = NULL;
	m_bConvErr = FALSE;
	m_hFileName = NULL;
}

AConverter::~AConverter()
{
	if (!m_bDone) // converter thread hasn't exited
	{
		WaitForConverter();
		m_nBytesAvail = 0;
		VERIFY(ResetEvent(m_hEventFile));
		m_nBytesAvail = 0;
		SetEvent(m_hEventConv);
		WaitForConverter();// wait for DoConversion exit
		VERIFY(ResetEvent(m_hEventFile));
	}

	if (m_hEventFile != NULL)
		VERIFY(CloseHandle(m_hEventFile));
	if (m_hEventConv != NULL)
		VERIFY(CloseHandle(m_hEventConv));
	if (m_hLibCnv != NULL)
		FreeLibrary(m_hLibCnv);
	if (m_hFileName != NULL)
		GlobalFree(m_hFileName);
}

void AConverter::WaitForConverter()
{
	// while event not signalled -- process messages
	while (MsgWaitForMultipleObjects(1, &m_hEventFile, FALSE, INFINITE,
		QS_SENDMESSAGE) != WAIT_OBJECT_0)
	{
		MSG msg;
		PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE);
	}
}

void AConverter::WaitForBuffer()
{
	// while event not signalled -- process messages
	while (MsgWaitForMultipleObjects(1, &m_hEventConv, FALSE, INFINITE,
		QS_SENDMESSAGE) != WAIT_OBJECT_0)
	{
		MSG msg;
		PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE);
	}
}

UINT AConverter::ConverterThread(LPVOID)
{
	ASSERT(m_pThis != NULL);

	HRESULT hRes = OleInitialize(NULL);
	VERIFY(hRes == S_OK || hRes == S_FALSE);
	m_pThis->DoConversion();
	OleUninitialize();

	return 0;
}

bool AConverter::IsFormatCorrect(LPCTSTR pszFileName)
{
	USES_CONVERSION;
	int nRet;
	if ((m_hLibCnv == NULL) || (m_pIsFormatCorrect == NULL))
   {
      ::AfxMessageBox(rStrMissingConverter);
		return false;
   }

	char buf[_MAX_PATH];
   ::strcpy_s(buf, mCountOf(buf), T2CA(pszFileName));

	CharToOemA(buf, buf);

	HGLOBAL hFileName = StringToHGLOBAL(buf);
	HGLOBAL hDesc = GlobalAlloc(GHND, 256);
	ASSERT(hDesc != NULL);
	nRet = m_pIsFormatCorrect(hFileName, hDesc);
	GlobalFree(hDesc);
	GlobalFree(hFileName);
	return (nRet == 1) ? true : false;
}

// static callback function
int CALLBACK AConverter::WriteOutStatic(int cch, int nPercentComplete)
{
	ASSERT(m_pThis != NULL);
	return m_pThis->WriteOut(cch, nPercentComplete);
}

int CALLBACK AConverter::WriteOut(int cch, int nPercentComplete)
{
	ASSERT(m_hBuff != NULL);
	m_nPercent = nPercentComplete;
	if (m_hBuff == NULL)
		return -9;
	if (cch != 0)
	{
		WaitForBuffer();
		VERIFY(ResetEvent(m_hEventConv));
		m_nBytesAvail = cch;
		SetEvent(m_hEventFile);
		WaitForBuffer();
	}
	return 0; //everything OK
}

int CALLBACK AConverter::ReadInStatic(int /*flags*/, int nPercentComplete)
{
	ASSERT(m_pThis != NULL);
	return m_pThis->ReadIn(nPercentComplete);
}

int CALLBACK AConverter::ReadIn(int /*nPercentComplete*/)
{
	ASSERT(m_hBuff != NULL);
	if (m_hBuff == NULL)
		return -8;

	SetEvent(m_hEventFile);
	WaitForBuffer();
	VERIFY(ResetEvent(m_hEventConv));

	return m_nBytesAvail;
}

BOOL AConverter::DoConversion()
{
	USES_CONVERSION;
	m_nLastPercent = -1;
	m_nPercent = 0;

	ASSERT(m_hBuff != NULL);
	ASSERT(m_pThis != NULL);
	HGLOBAL hDesc = StringToHGLOBAL("");
	HGLOBAL hSubset = StringToHGLOBAL("");

	int nRet = 0;
	if (m_bForeignToRtf)
	{
		ASSERT(m_pForeignToRtf != NULL);
		ASSERT(m_hFileName != NULL);
		nRet = m_pForeignToRtf(m_hFileName, NULL, m_hBuff, hDesc, hSubset,
			(LPFNOUT)WriteOutStatic);
		// wait for next AConverter::Read to come through
		WaitForBuffer();
		VERIFY(ResetEvent(m_hEventConv));
	}
	else
	{
		ASSERT(m_pRtfToForeign != NULL);
		ASSERT(m_hFileName != NULL);
		nRet = m_pRtfToForeign(m_hFileName, NULL, m_hBuff, hDesc,
			(LPFNIN)ReadInStatic);
		// don't need to wait for m_hEventConv
	}

	GlobalFree(hDesc);
	GlobalFree(hSubset);
	if (m_pBuf != NULL)
		GlobalUnlock(m_hBuff);
	GlobalFree(m_hBuff);

	if (nRet != 0)
		m_bConvErr = TRUE;

	m_bDone = TRUE;
	m_nPercent = 100;
	m_nLastPercent = -1;

	SetEvent(m_hEventFile);

	return (nRet == 0);
}

void AConverter::LoadFunctions()
{
	m_pInitConverter = (PINITCONVERTER)GetProcAddress(m_hLibCnv, "InitConverter32");
	m_pIsFormatCorrect = (PISFORMATCORRECT)GetProcAddress(m_hLibCnv, "IsFormatCorrect32");
	m_pForeignToRtf = (PFOREIGNTORTF)GetProcAddress(m_hLibCnv, "ForeignToRtf32");
	m_pRtfToForeign = (PRTFTOFOREIGN)GetProcAddress(m_hLibCnv, "RtfToForeign32");
}
#endif

BOOL AConverter::Open(LPCTSTR pszFileName, UINT nOpenFlags,
	CFileException* pException)
{
	USES_CONVERSION;
	// we convert to oem and back because of the following case
	// test(c).txt becomes testc.txt in OEM and stays testc.txt to Ansi
	char buf[_MAX_PATH];
   ::strcpy_s(buf, mCountOf(buf), T2CA(pszFileName));
	CharToOemA(buf, buf);
	OemToCharA(buf, buf);

	LPTSTR lpszFileNameT = A2T(buf);

	// let's make sure we could do what is wanted directly even though we aren't
	m_bCloseOnDelete = FALSE;
	m_hFile = hFileNull;

	BOOL bOpen = CFile::Open(lpszFileNameT, nOpenFlags, pException);
	CFile::Close();
	if (!bOpen)
		return FALSE;

	m_bForeignToRtf = !(nOpenFlags & (CFile::modeReadWrite | CFile::modeWrite));

	// check for reading empty file
	if (m_bForeignToRtf)
	{
		CFileStatus stat;
		if (CFile::GetStatus(lpszFileNameT, stat) && stat.m_size == 0)
			return TRUE;
	}

	//set security attributes to inherit handle
	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
	//create the events
	m_hEventFile = CreateEvent(&sa, TRUE, FALSE, NULL);
	m_hEventConv = CreateEvent(&sa, TRUE, FALSE, NULL);
	//create the converter thread and create the events

	CharToOemA(buf, buf);
	ASSERT(m_hFileName == NULL);
	m_hFileName = StringToHGLOBAL(buf);

	m_pThis = this;
	m_bDone = FALSE;
	m_hBuff = GlobalAlloc(GHND, BUFFSIZE);
	ASSERT(m_hBuff != NULL);

	AfxBeginThread(ConverterThread, this, THREAD_PRIORITY_NORMAL, 0, 0, &sa);

	return TRUE;
}

// m_hEventConv -- the main thread signals this event when ready for more data
// m_hEventFile -- the converter signals this event when data is ready

UINT AConverter::Read(void FAR* lpBuf, UINT nCount)
{
	ASSERT(m_bForeignToRtf);
	if (m_bDone)
		return 0;
	// if converter is done
	int cch = nCount;
	BYTE* pBuf = (BYTE*)lpBuf;
	while (cch != 0)
	{
		if (m_nBytesAvail == 0)
		{
			if (m_pBuf != NULL)
				GlobalUnlock(m_hBuff);
			m_pBuf = NULL;
			SetEvent(m_hEventConv);
			WaitForConverter();
			VERIFY(ResetEvent(m_hEventFile));
			if (m_bConvErr)
				AfxThrowFileException(CFileException::genericException);
			if (m_bDone)
				return nCount - cch;
			m_pBuf = (BYTE*)GlobalLock(m_hBuff);
			ASSERT(m_pBuf != NULL);
		}
		int nBytes = min(cch, m_nBytesAvail);
		memcpy(pBuf, m_pBuf, nBytes);
		pBuf += nBytes;
		m_pBuf += nBytes;
		m_nBytesAvail -= nBytes;
		cch -= nBytes;
		OutputPercent(m_nPercent);
	}
	return nCount - cch;
}

void AConverter::Write(const void FAR* lpBuf, UINT nCount)
{
	ASSERT(!m_bForeignToRtf);

	m_nBytesWritten += nCount;
	while (nCount != 0)
	{
		WaitForConverter();
		VERIFY(ResetEvent(m_hEventFile));
		if (m_bConvErr)
			AfxThrowFileException(CFileException::genericException);
		m_nBytesAvail = min(nCount, BUFFSIZE);
		nCount -= m_nBytesAvail;
		BYTE* pBuf = (BYTE*)GlobalLock(m_hBuff);
		ASSERT(pBuf != NULL);
		memcpy(pBuf, lpBuf, m_nBytesAvail);
		GlobalUnlock(m_hBuff);
		SetEvent(m_hEventConv);
	}
	OutputString(m_strSaving);
}

LONG AConverter::Seek(LONG lOff, UINT nFrom)
{
	if (lOff != 0 && nFrom != current)
		AfxThrowNotSupportedException();
	return 0;
}

CFile* AConverter::Duplicate() const
{
	AfxThrowNotSupportedException();
}

void AConverter::LockRange(DWORD, DWORD)
{
	AfxThrowNotSupportedException();
}

void AConverter::UnlockRange(DWORD, DWORD)
{
	AfxThrowNotSupportedException();
}

void AConverter::SetLength(DWORD)
{
	AfxThrowNotSupportedException();
}
