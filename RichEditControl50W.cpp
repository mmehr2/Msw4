// RichEditControl50W.cpp : implementation file
// By Jim Dunne http://www.topjimmy.net/tjs
// topjimmy@topjimmy.net
// copyright(C)2005
// if you use all or part of this code, please give me credit somewhere :)
#include "stdafx.h"
//#include "RE50W.h"
#include "RichEditControl50W.h"
#include ".\richeditcontrol50w.h"

// CRichEditControl50W
IMPLEMENT_DYNAMIC(CRichEditControl50W, CWnd)
CRichEditControl50W::CRichEditControl50W()
{
}

CRichEditControl50W::~CRichEditControl50W()
{
	//Free the MSFTEDIT.DLL library
	if(m_hInstRichEdit50W)
		FreeLibrary(m_hInstRichEdit50W);
}

BEGIN_MESSAGE_MAP(CRichEditControl50W, CWnd)
END_MESSAGE_MAP()

BOOL CRichEditControl50W::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	//Load the MSFTEDIT.DLL library
	m_hInstRichEdit50W = LoadLibrary(_T("msftedit.dll"));
	if (!m_hInstRichEdit50W)
	{
		AfxMessageBox(_T("MSFTEDIT.DLL Didn't Load"));
		return(0);
	}

	CWnd* pWnd = this;
	return pWnd->Create(_T("RichEdit50W"), NULL, dwStyle, rect, pParentWnd, nID);
}

void CRichEditControl50W::SetSel50W(long nStartChar, long nEndChar)
{
	m_crRE50W.cpMin = nStartChar;
	m_crRE50W.cpMax = nEndChar;
	SendMessage(EM_EXSETSEL, 0, (LPARAM)&m_crRE50W);
}

BOOL CRichEditControl50W::SetDefaultCharFormat50W(DWORD dwMask, COLORREF crTextColor, DWORD dwEffects, LPTSTR  szFaceName, LONG yHeight, COLORREF crBackColor)
{	//Set the text defaults.  CHARFORMAT2 m_cfStatus is declared in RichEditControl50W.h
	m_cfRE50W.cbSize = sizeof(CHARFORMAT2);
	m_cfRE50W.dwMask = dwMask ;
	m_cfRE50W.crTextColor = crTextColor;
	m_cfRE50W.dwEffects = dwEffects;
	::lstrcpy(m_cfRE50W.szFaceName, szFaceName);
	m_cfRE50W.yHeight = yHeight;
	m_cfRE50W.crBackColor = crBackColor;

	return (BOOL) SendMessage(EM_SETCHARFORMAT, 0, (LPARAM)&m_cfRE50W);
}

void CRichEditControl50W::SetTextTo50WControl(CString csText, int nSTFlags, int nSTCodepage)
{	//Set the options. SETTEXTEX m_st50W declared in RichEditControl50W.h
	m_st50W.codepage = nSTCodepage;	
	m_st50W.flags = nSTFlags;
	SendMessage(EM_SETTEXTEX, (WPARAM)&m_st50W, (LPARAM)(LPCTSTR)csText);
}
void CRichEditControl50W::LimitText50W(int nChars)
{
	SendMessage(EM_LIMITTEXT, nChars, 0);
}

void CRichEditControl50W::SetOptions50W(WORD wOp, DWORD dwFlags)
{
	SendMessage(EM_SETOPTIONS, (WPARAM)wOp, (LPARAM)dwFlags);
}

DWORD CRichEditControl50W::SetEventMask50W(DWORD dwEventMask)
{
	return (DWORD)SendMessage(EM_SETEVENTMASK, 0, dwEventMask);
}

void CRichEditControl50W::GetTextRange50W(int ncharrMin, int ncharrMax)
{
	//Set the CHARRANGE for the trRE50W = the characters sent by ENLINK 
	m_trRE50W.chrg.cpMin = ncharrMin;
	m_trRE50W.chrg.cpMax = ncharrMax;

	//Set the size of the character buffers, + 1 for null character
	int nLength = int((m_trRE50W.chrg.cpMax - m_trRE50W.chrg.cpMin +1));

	//create an ANSI buffer and a Unicode (Wide Character) buffer
	m_lpszChar = new CHAR[nLength];
	LPWSTR lpszWChar = new WCHAR[nLength];

	//Set the trRE50W LPWSTR character buffer = Unicode buffer
	m_trRE50W.lpstrText = lpszWChar;

	//Get the Unicode text
	SendMessage(EM_GETTEXTRANGE, 0,  (LPARAM) &m_trRE50W);  

	// Convert the Unicode RTF text to ANSI.
	WideCharToMultiByte(CP_ACP, 0, lpszWChar, -1, m_lpszChar, nLength, NULL, NULL);

	//Release buffer memory
	delete lpszWChar;

	return;
}

void CRichEditControl50W::GetSel(long& nStartChar, long& nEndChar) const
{
	ASSERT(::IsWindow(m_hWnd));
	CHARRANGE cr;
	::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
	nStartChar = cr.cpMin;
	nEndChar = cr.cpMax;
}

int CRichEditControl50W::GetTextRange(int nFirst, int nLast,
	CString& refString) const
{
	ASSERT(::IsWindow(m_hWnd));

	TEXTRANGE textRange;
	textRange.chrg.cpMin = nFirst;
	textRange.chrg.cpMax = nLast;

	// can't be backwards
	int nLength = int(nLast - nFirst + 1);
	ASSERT(nLength > 0);

	textRange.lpstrText = refString.GetBuffer(nLength);
	nLength = (int)::SendMessage(m_hWnd, EM_GETTEXTRANGE, 0, (LPARAM) &textRange);
	refString.ReleaseBuffer(nLength);

	return (int)nLength;
}

int CRichEditControl50W::GetLine(int nIndex, __out LPTSTR lpszBuffer) const
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)::SendMessage(m_hWnd, EM_GETLINE, nIndex,
		(LPARAM)lpszBuffer);
}

int CRichEditControl50W::LineLength(int nLine /* = -1 */) const
{
	ASSERT(::IsWindow(m_hWnd));
	return (int)::SendMessage(m_hWnd, EM_LINELENGTH, nLine, 0);
}

void CRichEditControl50W::LineScroll(int nLines, int nChars /* = 0 */)
{
	ASSERT(::IsWindow(m_hWnd));
	::SendMessage(m_hWnd, EM_LINESCROLL, nChars, nLines);
}

BOOL CRichEditControl50W::CanPaste(UINT nFormat) const
{
	ASSERT(::IsWindow(m_hWnd));
	COleMessageFilter* pFilter = AfxOleGetMessageFilter();
	if (pFilter != NULL)
		pFilter->BeginBusyState();
	BOOL b = (BOOL)::SendMessage(m_hWnd, EM_CANPASTE, nFormat, 0L);
	if (pFilter != NULL)
		pFilter->EndBusyState();
	return b;
}

void CRichEditControl50W::PasteSpecial(UINT nClipFormat, DWORD dvAspect, HMETAFILE hMF)
{
	ASSERT(::IsWindow(m_hWnd));
	REPASTESPECIAL reps;
	reps.dwAspect = dvAspect;
	reps.dwParam = (DWORD_PTR)hMF;
	::SendMessage(m_hWnd, EM_PASTESPECIAL, nClipFormat, (LPARAM)&reps);
}

int CRichEditControl50W::GetLine(int nIndex, __out_ecount(nMaxLength) LPTSTR lpszBuffer, int nMaxLength) const
{
	ASSERT(::IsWindow(m_hWnd));
	ENSURE(sizeof(nMaxLength)<=nMaxLength*sizeof(TCHAR)&&nMaxLength>0);
	*(LPINT)lpszBuffer = nMaxLength;
	return (int)::SendMessage(m_hWnd, EM_GETLINE, nIndex, (LPARAM)lpszBuffer);
}

CString CRichEditControl50W::GetSelText() const
{
	ASSERT(::IsWindow(m_hWnd));
	CHARRANGE cr;
	cr.cpMin = cr.cpMax = 0;
	::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr);
    CStringA strText;
    LPSTR lpsz=strText.GetBufferSetLength((cr.cpMax - cr.cpMin + 1)*2); 
	lpsz[0] = NULL;
	::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpsz);
    strText.ReleaseBuffer();
	return CString(strText);
}

IRichEditOle* CRichEditControl50W::GetIRichEditOle() const
{
	ASSERT(::IsWindow(m_hWnd));
	IRichEditOle *pRichItem = NULL;
	::SendMessage(m_hWnd, EM_GETOLEINTERFACE, 0, (LPARAM)&pRichItem);
	return pRichItem;
}

long CRichEditControl50W::GetTextLengthEx(DWORD dwFlags,
	UINT nCodePage /* = -1 */) const
{
	ASSERT(::IsWindow(m_hWnd));
	GETTEXTLENGTHEX textLenEx;
	textLenEx.flags = dwFlags;

	if (nCodePage == -1)
	{
#ifdef _UNICODE
		// UNICODE code page
		textLenEx.codepage = 1200;
#else
		// default code page
		textLenEx.codepage = CP_ACP;
#endif
	}
	else
		// otherwise, use the code page specified
		textLenEx.codepage = nCodePage;

	return (long) ::SendMessage(m_hWnd, EM_GETTEXTLENGTHEX,
		(WPARAM) &textLenEx, 0);
}

BOOL CRichEditControl50W::SetDefaultCharFormat(CHARFORMAT &cf)
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, 0, (LPARAM)&cf);
}

BOOL CRichEditControl50W::SetDefaultCharFormat(CHARFORMAT2 &cf)
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT2);
	return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, 0, (LPARAM)&cf);
}

BOOL CRichEditControl50W::SetSelectionCharFormat(CHARFORMAT &cf)
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

BOOL CRichEditControl50W::SetSelectionCharFormat(CHARFORMAT2 &cf)
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT2);
	return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

BOOL CRichEditControl50W::SetWordCharFormat(CHARFORMAT &cf)
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION|SCF_WORD, (LPARAM)&cf);
}

BOOL CRichEditControl50W::SetWordCharFormat(CHARFORMAT2 &cf)
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT2);
	return (BOOL)::SendMessage(m_hWnd, EM_SETCHARFORMAT, SCF_SELECTION|SCF_WORD, (LPARAM)&cf);
}

DWORD CRichEditControl50W::GetDefaultCharFormat(CHARFORMAT &cf) const
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 0, (LPARAM)&cf);
}

DWORD CRichEditControl50W::GetDefaultCharFormat(CHARFORMAT2 &cf) const
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT2);
	return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 0, (LPARAM)&cf);
}

DWORD CRichEditControl50W::GetSelectionCharFormat(CHARFORMAT &cf) const
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT);
	return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 1, (LPARAM)&cf);
}

DWORD CRichEditControl50W::GetSelectionCharFormat(CHARFORMAT2 &cf) const
{
	ASSERT(::IsWindow(m_hWnd));
	cf.cbSize = sizeof(CHARFORMAT2);
	return (DWORD)::SendMessage(m_hWnd, EM_GETCHARFORMAT, 1, (LPARAM)&cf);
}

DWORD CRichEditControl50W::GetParaFormat(PARAFORMAT &pf) const
{
	ASSERT(::IsWindow(m_hWnd));
	pf.cbSize = sizeof(PARAFORMAT);
	return (DWORD)::SendMessage(m_hWnd, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
}

DWORD CRichEditControl50W::GetParaFormat(PARAFORMAT2 &pf) const
{
	ASSERT(::IsWindow(m_hWnd));
	pf.cbSize = sizeof(PARAFORMAT2);
	return (DWORD)::SendMessage(m_hWnd, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
}

BOOL CRichEditControl50W::SetParaFormat(PARAFORMAT &pf)
{
	ASSERT(::IsWindow(m_hWnd));
	pf.cbSize = sizeof(PARAFORMAT);
	return (BOOL)::SendMessage(m_hWnd, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
}

BOOL CRichEditControl50W::SetParaFormat(PARAFORMAT2 &pf)
{
	ASSERT(::IsWindow(m_hWnd));
	pf.cbSize = sizeof(PARAFORMAT2);
	return (BOOL)::SendMessage(m_hWnd, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
}
