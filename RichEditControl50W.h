// By Jim Dunne http://www.topjimmy.net/tjs
// topjimmy@topjimmy.net
// copyright(C)2005
// if you use all or part of this code, please give me credit somewhere :)
#pragma once
// CRichEditControl50W
class CRichEditControl50W : public CWnd
{
	DECLARE_DYNAMIC(CRichEditControl50W)

protected:
	DECLARE_MESSAGE_MAP()
	CHARRANGE m_crRE50W;
	CHARFORMAT2 m_cfRE50W;
	SETTEXTEX m_st50W;

// Constructors
public:
	CRichEditControl50W();
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	//virtual BOOL CreateEx(DWORD dwExStyle, DWORD dwStyle, const RECT& rect,
	//	CWnd* pParentWnd, UINT nID);

// Attributes
	HINSTANCE m_hInstRichEdit50W;      // handle to MSFTEDIT.DLL
	TEXTRANGEW m_trRE50W;	//TextRangeW structure, for Unicode
	LPSTR m_lpszChar;

	void SetSel50W(long nStartChar, long nEndChar);
	BOOL SetDefaultCharFormat50W(DWORD dwMask, COLORREF crTextColor, DWORD dwEffects, LPTSTR szFaceName, LONG yHeight, COLORREF crBackColor);
	void SetTextTo50WControl(CString csText, int nSTFlags, int nSTCodepage);
	void LimitText50W(int nChars);
	void SetOptions50W(WORD wOp, DWORD dwFlags);
	DWORD SetEventMask50W(DWORD dwEventMask);
	void GetTextRange50W(int ncharrMin, int ncharrMax);

	virtual ~CRichEditControl50W();

   BOOL CanUndo() const
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_CANUNDO, 0, 0); }
   BOOL CanRedo() const
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_CANREDO, 0, 0); }
   UNDONAMEID GetUndoName() const
	   { ASSERT(::IsWindow(m_hWnd)); return (UNDONAMEID) ::SendMessage(m_hWnd, EM_GETUNDONAME, 0, 0); }
   UNDONAMEID GetRedoName() const
	   { ASSERT(::IsWindow(m_hWnd)); return (UNDONAMEID) ::SendMessage(m_hWnd, EM_GETREDONAME, 0, 0); }
   int GetLineCount() const
	   { ASSERT(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, EM_GETLINECOUNT, 0, 0); }
   BOOL GetModify() const
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_GETMODIFY, 0, 0); }
   void SetModify(BOOL bModified /* = TRUE */)
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_SETMODIFY, bModified, 0);}
   BOOL SetTextMode(UINT fMode)
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_SETTEXTMODE, (WPARAM) fMode, 0); }
   UINT GetTextMode() const
	   { ASSERT(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_GETTEXTMODE, 0, 0); }
   void GetRect(LPRECT lpRect) const
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_GETRECT, 0, (LPARAM)lpRect); }
   CPoint GetCharPos(long lChar) const
	   { ASSERT(::IsWindow(m_hWnd)); CPoint pt; ::SendMessage(m_hWnd, EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)lChar); return pt;}
   UINT GetOptions() const
	   { ASSERT(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_GETOPTIONS, 0, 0); }
   void SetOptions(WORD wOp, DWORD dwFlags)
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_SETOPTIONS, (WPARAM)wOp, (LPARAM)dwFlags); }
   BOOL SetAutoURLDetect(BOOL bEnable /* = TRUE */)
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_AUTOURLDETECT, (WPARAM) bEnable, 0); }
   void EmptyUndoBuffer()
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EMPTYUNDOBUFFER, 0, 0); }
   UINT SetUndoLimit(UINT nLimit)
	   { ASSERT(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_SETUNDOLIMIT, (WPARAM) nLimit, 0); }
   void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE)
	   {ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_REPLACESEL, (WPARAM) bCanUndo, (LPARAM)lpszNewText); }
   void SetRect(LPCRECT lpRect)
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_SETRECT, 0, (LPARAM)lpRect); }
   void StopGroupTyping()
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_STOPGROUPTYPING, 0, 0); }
   BOOL Redo()
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_REDO, 0, 0); }
   BOOL Undo()
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_UNDO, 0, 0); }
   void Clear()
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_CLEAR, 0, 0); }
   void Copy()
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_COPY, 0, 0); }
   void Cut()
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_CUT, 0, 0); }
   void Paste()
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, WM_PASTE, 0, 0); }
   BOOL SetReadOnly(BOOL bReadOnly /* = TRUE */ )
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETREADONLY, bReadOnly, 0L); }
   int GetFirstVisibleLine() const
	   { ASSERT(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, EM_GETFIRSTVISIBLELINE, 0, 0L); }
   BOOL DisplayBand(LPRECT pDisplayRect)
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_DISPLAYBAND, 0, (LPARAM)pDisplayRect); }
   void GetSel(CHARRANGE &cr) const
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EXGETSEL, 0, (LPARAM)&cr); }
   BOOL GetPunctuation(UINT fType, PUNCTUATION* lpPunc) const
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_GETPUNCTUATION, (WPARAM) fType, (LPARAM) lpPunc); }
   BOOL SetPunctuation(UINT fType, PUNCTUATION* lpPunc)
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, EM_SETPUNCTUATION, (WPARAM) fType, (LPARAM) lpPunc); }
   void LimitText(long nChars)
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EXLIMITTEXT, 0, nChars); }
   long LineFromChar(long nIndex) const
	   { ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_EXLINEFROMCHAR, 0, nIndex); }
   CPoint PosFromChar(UINT nChar) const
	   { ASSERT(::IsWindow(m_hWnd)); POINTL pt; ::SendMessage(m_hWnd, EM_POSFROMCHAR, (WPARAM)&pt, nChar); return CPoint(pt.x, pt.y); }
   int CharFromPos(CPoint pt) const
	   { ASSERT(::IsWindow(m_hWnd)); POINTL ptl = {pt.x, pt.y}; return (int)::SendMessage(m_hWnd, EM_CHARFROMPOS, 0, (LPARAM)&ptl); }
   void SetSel(CHARRANGE &cr)
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_EXSETSEL, 0, (LPARAM)&cr); }
   void SetSel(long nStartChar, long nEndChar)
      { ASSERT(::IsWindow(m_hWnd)); CHARRANGE cr; cr.cpMin = nStartChar; cr.cpMax = nEndChar; ::SendMessage(m_hWnd, EM_EXSETSEL, 0, (LPARAM)&cr); }
   DWORD FindWordBreak(UINT nCode, DWORD nStart) const
	   { ASSERT(::IsWindow(m_hWnd)); return (DWORD)::SendMessage(m_hWnd, EM_FINDWORDBREAK, (WPARAM) nCode, (LPARAM) nStart); }
   long FindText(DWORD dwFlags, FINDTEXTEX* pFindText) const
	   { ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_FINDTEXTEX, dwFlags, (LPARAM)pFindText); }
   long FormatRange(FORMATRANGE* pfr, BOOL bDisplay)
	   { ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_FORMATRANGE, (WPARAM)bDisplay, (LPARAM)pfr); }
   long GetEventMask() const
	   { ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_GETEVENTMASK, 0, 0L); }
   long GetLimitText() const
	   { ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_GETLIMITTEXT, 0, 0L); }
   long GetSelText(__out_z LPSTR lpBuf) const
	   { ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_GETSELTEXT, 0, (LPARAM)lpBuf); }
   void HideSelection(BOOL bHide, BOOL bPerm)
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_HIDESELECTION, bHide, bPerm); }
   void RequestResize()
	   { ASSERT(::IsWindow(m_hWnd)); ::SendMessage(m_hWnd, EM_REQUESTRESIZE, 0, 0L); }
   WORD GetSelectionType() const
	   { ASSERT(::IsWindow(m_hWnd)); return (WORD)::SendMessage(m_hWnd, EM_SELECTIONTYPE, 0, 0L); }
   UINT GetWordWrapMode() const
	   { ASSERT(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_GETWORDWRAPMODE, 0, 0); }
   UINT SetWordWrapMode(UINT uFlags) const
	   { ASSERT(::IsWindow(m_hWnd)); return (UINT) ::SendMessage(m_hWnd, EM_SETWORDWRAPMODE, (WPARAM) uFlags, 0); }
   COLORREF SetBackgroundColor(BOOL bSysColor, COLORREF cr)
	   { ASSERT(::IsWindow(m_hWnd)); return (COLORREF)::SendMessage(m_hWnd, EM_SETBKGNDCOLOR, bSysColor, cr); }
   DWORD SetEventMask(DWORD dwEventMask)
	   { ASSERT(::IsWindow(m_hWnd)); return (DWORD)::SendMessage(m_hWnd, EM_SETEVENTMASK, 0, dwEventMask); }
   BOOL SetOLECallback(IRichEditOleCallback* pCallback)
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETOLECALLBACK, 0, (LPARAM)pCallback); }
   BOOL SetTargetDevice(HDC hDC, long lLineWidth)
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETTARGETDEVICE, (WPARAM)hDC, lLineWidth); }
   BOOL SetTargetDevice(CDC &dc, long lLineWidth)
	   { ASSERT(::IsWindow(m_hWnd)); return (BOOL)::SendMessage(m_hWnd, EM_SETTARGETDEVICE, (WPARAM)dc.m_hDC, lLineWidth); }
   long StreamIn(int nFormat, EDITSTREAM &es)
	   { ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_STREAMIN, nFormat, (LPARAM)&es); }
   long StreamOut(int nFormat, EDITSTREAM &es)
	   { ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, EM_STREAMOUT, nFormat, (LPARAM)&es); }
   long GetTextLength() const
	   { ASSERT(::IsWindow(m_hWnd)); return (long)::SendMessage(m_hWnd, WM_GETTEXTLENGTH, NULL, NULL); }
   int LineIndex(int nLine = -1) const 
      { ASSERT(::IsWindow(m_hWnd)); return (int)::SendMessage(m_hWnd, EM_LINEINDEX, nLine, 0); }

   void GetSel(long& nStartChar, long& nEndChar) const;
   int GetTextRange(int nFirst, int nLast, CString& refString) const;
   BOOL CreateEx(DWORD dwExStyle, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
   int GetLine(int nIndex, __out LPTSTR lpszBuffer) const;
   int LineLength(int nLine = -1) const;
   void LineScroll(int nLines, int nChars = 0);
   BOOL CanPaste(UINT nFormat = 0) const;
   void PasteSpecial(UINT nClipFormat, DWORD dvAspect, HMETAFILE hMF);
   int GetLine(int nIndex, __out_ecount(nMaxLength) LPTSTR lpszBuffer, int nMaxLength) const;
   CString GetSelText() const;
   IRichEditOle* GetIRichEditOle() const;
   long GetTextLengthEx(DWORD dwFlags, UINT nCodePage = -1) const;
   BOOL SetDefaultCharFormat(CHARFORMAT &cf);
   BOOL SetDefaultCharFormat(CHARFORMAT2 &cf);
   BOOL SetSelectionCharFormat(CHARFORMAT &cf);
   BOOL SetSelectionCharFormat(CHARFORMAT2 &cf);
   BOOL SetWordCharFormat(CHARFORMAT &cf);
   BOOL SetWordCharFormat(CHARFORMAT2 &cf);
   DWORD GetDefaultCharFormat(CHARFORMAT &cf) const;
   DWORD GetDefaultCharFormat(CHARFORMAT2 &cf) const;
   DWORD GetSelectionCharFormat(CHARFORMAT &cf) const;
   DWORD GetSelectionCharFormat(CHARFORMAT2 &cf) const;
   DWORD GetParaFormat(PARAFORMAT &pf) const;
   DWORD GetParaFormat(PARAFORMAT2 &pf) const;
   BOOL SetParaFormat(PARAFORMAT &pf);
   BOOL SetParaFormat(PARAFORMAT2 &pf);
};
