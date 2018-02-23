// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"

#include "doctype.h"
#include "strings.h"
#include "multconv.h"
#include "resource.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static bool IsConverterFormat(LPCTSTR pszConverter, LPCTSTR pszPathName);
static bool IsWord6(LPCTSTR pszConverter, LPCTSTR pszPathName);
static bool IsDllInPath(LPCTSTR lpszName);

DocType gDocTypes[] =
{
	DECLARE_DOCTYPE(WINWORD2, FALSE, FALSE, FALSE, NULL),
	DECLARE_DOCTYPE(WINWORD6, TRUE, FALSE, FALSE, gStrWordConverter),
	DECLARE_DOCTYPE_SYN(MSW, RICHTEXT, TRUE, TRUE, TRUE, NULL),
	DECLARE_DOCTYPE(RICHTEXT, TRUE, TRUE, FALSE, NULL),
	DECLARE_DOCTYPE(URICHTEXT, TRUE, TRUE, FALSE, NULL),
	DECLARE_DOCTYPE(TEXT, TRUE, TRUE, FALSE, NULL),
	DECLARE_DOCTYPE(UTEXT, TRUE, TRUE, FALSE, NULL),
	DECLARE_DOCTYPE(OEMTEXT, TRUE, TRUE, FALSE, NULL),
	DECLARE_DOCTYPE(ALL, TRUE, FALSE, FALSE, NULL),
};

CString DocType::GetString(int nID)
{
	ASSERT(idStr != NULL);
	CString str;
	VERIFY(str.LoadString(idStr));
	CString strSub;
	AfxExtractSubString(strSub, str, nID);
	return strSub;
}

static bool IsConverterFormat(LPCTSTR pszConverter, LPCTSTR pszPathName)
{
   ASSERT(NULL != pszConverter);
   ASSERT(NULL != pszPathName);
	AConverter conv(pszConverter);
	return conv.IsFormatCorrect(pszPathName);
}

static bool IsLeadMatch(CFile& file, const BYTE* pb, UINT nCount)
{  // check for match at beginning of file
	bool match = FALSE;
	BYTE* buf = new BYTE[nCount];

	TRY
	{
		file.SeekToBegin();
		memset(buf, 0, nCount);
		file.Read(buf, nCount);
		match = (memcmp(buf, pb, nCount) == 0);
	}
	END_TRY

	delete [] buf;
	return match;
}

static bool IsWord6(LPCTSTR pszPathName)
{
	USES_CONVERSION;
	bool result = false;
	// see who created it
	LPSTORAGE lpStorage;
	SCODE sc = StgOpenStorage(T2COLE(pszPathName), NULL,
		STGM_READ|STGM_SHARE_EXCLUSIVE, 0, 0, &lpStorage);
	if (sc == NOERROR)
	{
		LPSTREAM lpStream;
		sc = lpStorage->OpenStream(T2COLE(gStrSumInfo), NULL,
			STGM_READ|STGM_SHARE_EXCLUSIVE, NULL, &lpStream);
		if (sc == NOERROR)
		{
			lpStream->Release();
			result = true;
		}
		lpStorage->Release();
	}
	return result;
}

DocType::Id GetDocTypeFromName(LPCTSTR pszPathName, DocType::Id& supposedType, CFileException& fe)
{
	CFile file;
	ASSERT(pszPathName != NULL);

	if (!file.Open(pszPathName, CFile::modeRead | CFile::shareDenyWrite, &fe))
      return DocType::RD_UNKNOWN;

	CFileStatus stat;
	VERIFY(file.GetStatus(stat));

	if (stat.m_size == 0) // file is empty
	{
		CString ext = CString(pszPathName).Right(4);
		if (ext[0] != '.')
			return supposedType = DocType::RD_TEXT;
		if (lstrcmpi(ext, _T(".doc"))==0)
			return supposedType = DocType::RD_MSW;
		if (lstrcmpi(ext, _T(".rtf"))==0)
			return supposedType = DocType::RD_RICHTEXT;
		return supposedType = DocType::RD_TEXT;
	}
	// RTF
	if (IsLeadMatch(file, sRtfPrefix, sizeof(sRtfPrefix)))
		return supposedType = DocType::RD_RICHTEXT;
   // Unicode Rtf
	else if (IsLeadMatch(file, (BYTE*)sURtfPrefix, sizeof(sURtfPrefix)))
		return supposedType = DocType::RD_URICHTEXT;
	// WORD 2
	else if (IsLeadMatch(file, sWord2Prefix, sizeof(sWord2Prefix)))
		return supposedType = DocType::RD_WINWORD2;
	// Unicode Text
	else if (IsLeadMatch(file, sUTxt, sizeof(sUTxt)))
		return supposedType = DocType::RD_UTEXT;

	// test for compound file
	if (IsLeadMatch(file, sCompFilePrefix, sizeof(sCompFilePrefix)))
	{
		file.Close();
      supposedType = DocType::RD_WINWORD6;
		if (IsConverterFormat(gStrWordConverter, pszPathName))
		{
			if (IsWord6(pszPathName))
				return supposedType = DocType::RD_WINWORD6;
			else
				return supposedType = DocType::RD_MSW;
		}
		return DocType::RD_TEXT;
	}
	return DocType::RD_TEXT;
}

void ScanForConverters()
{
	static bool bScanned = FALSE;
	if (!bScanned)
   {
	   for (int i = 0; i < mCountOf(gDocTypes); i++)
	   {
		   LPCTSTR lpsz = gDocTypes[i].pszConverterName;
		   // if converter specified but can't find it
		   if (lpsz && *lpsz && !IsDllInPath(lpsz))
			   gDocTypes[i].bRead = gDocTypes[i].bWrite = FALSE;
	   }
	   if (GetSystemMetrics(SM_DBCSENABLED))
		   gDocTypes[DocType::RD_OEMTEXT].bRead = 
          gDocTypes[DocType::RD_OEMTEXT].bWrite = FALSE;

	   bScanned = true;
   }
}

bool IsDllInPath(LPCTSTR lpszName)
{
	ASSERT(lpszName != NULL);
   WIN32_FIND_DATA data;
   return (INVALID_HANDLE_VALUE != ::FindFirstFile(lpszName, &data));
}

CString GetExtFromType(DocType::Id nDocType)
{
	ScanForConverters();

	CString str = gDocTypes[nDocType].GetString(DOCTYPE_EXT);
	if (!str.IsEmpty())
	{
		ASSERT(str.GetLength() > 2); // "*.ext, *.*"
		ASSERT(str[1] == '.');
		return str.Right(str.GetLength()-1);
	}
	return str;
}

// returns an RD_* from an index into the openfile dialog types
DocType::Id GetTypeFromIndex(int nIndex, bool bOpen)
{
	ScanForConverters();

	int nCnt = 0;
	for (int i = 0; i < mCountOf(gDocTypes); i++)
	{
		if (!gDocTypes[i].bDup &&
			(bOpen ? gDocTypes[i].bRead : gDocTypes[i].bWrite))
		{
			if (nCnt == nIndex)
				return (DocType::Id)i;
			nCnt++;
		}
	}
	ASSERT(FALSE);
	return DocType::RD_UNKNOWN;
}

// returns an index into the openfile dialog types for the RD_* type
int GetIndexFromType(DocType::Id nType, bool bOpen)
{
	ScanForConverters();

	int nCnt = 0;
	for (int i = 0; i < mCountOf(gDocTypes); i++)
	{
		if (!gDocTypes[i].bDup &&
			(bOpen ? gDocTypes[i].bRead : gDocTypes[i].bWrite))
		{
			if (i == nType)
				return nCnt;
			nCnt++;
		}
	}
	return -1;
}

CString GetFileTypes(bool bOpen)
{
	ScanForConverters();

	CString str;
	for (int i = 0; i < mCountOf(gDocTypes); i++)
	{
		if (bOpen && gDocTypes[i].bRead && !gDocTypes[i].bDup)
		{
			str += gDocTypes[i].GetString(DOCTYPE_DESC);
			str += (TCHAR)NULL;
			str += gDocTypes[i].GetString(DOCTYPE_EXT);
			str += (TCHAR)NULL;
		}
		else if (!bOpen && gDocTypes[i].bWrite && !gDocTypes[i].bDup)
		{
			str += gDocTypes[i].GetString(DOCTYPE_DOCTYPE);
			str += (TCHAR)NULL;
			str += gDocTypes[i].GetString(DOCTYPE_EXT);
			str += (TCHAR)NULL;
		}
	}
	return str;
}
