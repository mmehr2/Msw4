// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "RtfHelper.h"
#include "Msw.h"
#include "Ruler.h"

class AMswDoc;
class CRtfMemFile;

class AMswView : public CRichEditView
{
public:  // methods
   AMswDoc* GetDocument();
	BOOL IsFormatText();

	virtual HMENU GetContextMenu(WORD seltype, LPOLEOBJECT lpoleobj, CHARRANGE* lpchrg);

	void SetDefaultFont();
	void SetUpdateTimer();
	void DrawMargins(CDC* pDC);
	BOOL SelectPalette();

   void SetSel(long start, long end);
   void GetSel(long& start, long& end);
   void ReplaceSel(CString& text);
   CString GetSelText();

   void Serialize(CArchive& ar);
   void Stream(CArchive& ar, BOOL bSelection);

   DWORD GetErt() const;
   DWORD GetArt() const;
   void SetArt(DWORD art);

public:  // data
   ARtfHelper fRtfHelper;
	BOOL m_bDelayUpdateItems;
	BOOL m_bInPrint;

protected: // create from serialization only
	AMswView();
	virtual ~AMswView();
	DECLARE_DYNCREATE(AMswView)

   //{{AFX_VIRTUAL(AMswView)
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(AMswView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPageSetup();
	afx_msg void OnFormatParagraph();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnFilePrint();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCheckSpelling();
	afx_msg void OnSpellerOptions();
	afx_msg void OnUpdateSpellerOptions(CCmdUI* pCmdUI);
   afx_msg void OnAutoCorrectOptions();
   afx_msg void OnUpdateAutoCorrectOptions(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCheckSpelling(CCmdUI* pCmdUI);
	afx_msg void OnEditChange();
	afx_msg void OnColorPick(UINT nID);
	afx_msg void OnCuePointer(UINT nID);
	afx_msg void OnSpellCorrection(UINT nID);
	afx_msg int OnMouseActivate(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg LONG OnPrinterChangedMsg(UINT, LONG);
	afx_msg void OnGetCharFormat(NMHDR* pNMHDR, LRESULT* pRes);
	afx_msg void OnSetCharFormat(NMHDR* pNMHDR, LRESULT* pRes);
	afx_msg void OnBarSetFocus(NMHDR*, LRESULT*);
	afx_msg void OnBarKillFocus(NMHDR*, LRESULT*);
	afx_msg void OnBarReturn(NMHDR*, LRESULT* );
   afx_msg void OnFontScale();
   afx_msg void OnFormatStyle();

   afx_msg void OnGotoBookmark(UINT uId);
   afx_msg void OnStyle(UINT uId);
   afx_msg void OnSetBookmark();
   afx_msg void OnUpdateSetBookmark(CCmdUI *pCmdUI);
   afx_msg void OnClearBookmarks();
   afx_msg void OnUpdateClearBookmarks(CCmdUI *pCmdUI);
   afx_msg void OnNextBookmark();
   afx_msg void OnUpdateNextBookmark(CCmdUI *pCmdUI);
   afx_msg void OnPrevBookmark();
   afx_msg void OnUpdatePrevBookmark(CCmdUI *pCmdUI);

   afx_msg void OnSetPaperclip();
   afx_msg void OnUpdateSetPaperclip(CCmdUI *pCmdUI);
   afx_msg void OnClearPaperclips();
   afx_msg void OnUpdateClearPaperclips(CCmdUI *pCmdUI);
   afx_msg void OnNextPaperclip();
   afx_msg void OnUpdateNextPaperclip(CCmdUI *pCmdUI);
   afx_msg void OnPrevPaperclip();
   afx_msg void OnUpdatePrevPaperclip(CCmdUI *pCmdUI);
   afx_msg void OnGotoLine();
   afx_msg void OnEnChange();
   afx_msg void OnEnSelchange(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnPaint();
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
   afx_msg void OnSysColorChange();
   afx_msg void OnEnVscroll();
   afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnUpdateBookmarks(CCmdUI *pCmdUI);
   afx_msg void OnBookmarks();
   afx_msg void OnFindReplace();
   afx_msg void OnFormatFont();
   afx_msg void OnSpacing();
   afx_msg void OnStylePlain();
   afx_msg void OnSize();
	afx_msg void OnUpdateSize(CCmdUI* pCmdUI);
   afx_msg void OnSizeOther();
   afx_msg LONG OnShowRuler(UINT, LONG);
   afx_msg LONG OnSetScrollMargins(UINT, LONG);
   afx_msg LONG OnRedrawCueBar(UINT, LONG);
   afx_msg LONG OnInverseEditMode(UINT, LONG);
   afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnEditPaste();
	afx_msg void OnEditPasteSpecial();
   afx_msg void OnUpdatePaste(CCmdUI* pCmdUI);
   afx_msg void OnUpdateColor(CCmdUI *pCmdUI);
   //}}AFX_MSG

   BOOL OnPreparePrinting(CPrintInfo* pInfo);
   BOOL CanPaste() const;

	virtual void DeleteContents();
	virtual void OnTextNotFound(LPCTSTR);
   virtual void OnInitialUpdate();
   virtual void OnReplaceSel(LPCTSTR lpszFind, BOOL bNext, BOOL bCase, BOOL bWord, LPCTSTR lpszReplace);
   virtual void OnReplaceAll(LPCTSTR lpszFind, LPCTSTR lpszReplace, BOOL bCase, BOOL bWord);
	BOOL CreateRulerBar();

	DECLARE_MESSAGE_MAP()

   void ReadMarks(CRtfMemFile& file);
   void WriteMarks(CRtfMemFile& file);
   BOOL SameAsSelected(LPCTSTR lpszCompare, BOOL bCase, BOOL /*bWord*/);
   void ToggleTextColor(COLORREF newColor);
   void SetDragSelect(bool selecting);
   afx_msg void OnSelectAll();
   afx_msg void OnTimerToArt();
   afx_msg void OnScrollLineUp();
   afx_msg void OnScrollLineDown();

private: // data
	BOOL m_bOnBar;
   int fTextLength;
	CHARRANGE fSelectedText;
   DWORD fArt;
   UINT m_uTimerID, fAutoScrollTimer;
	ARulerBar   m_wndRulerBar;
	CParaFormat m_defParaFormat;
   int fRulerHeight;

   bool fDragSelecting;
   int fSelStart;
};

//-----------------------------------------------------------------------
// inline functions
inline void AMswView::SetSel(long start, long end)
{
   this->GetRichEditCtrl().SetSel(start, end);
}

inline void AMswView::GetSel(long& start, long& end)
{
   this->GetRichEditCtrl().GetSel(start, end);
}

inline void AMswView::ReplaceSel(CString& text)
{
   this->GetRichEditCtrl().ReplaceSel(text);
}

inline DWORD AMswView::GetErt() const
{
   const long len = this->GetTextLength() * 10;
   return (len / theApp.fCharsPerSecond);
}

inline DWORD AMswView::GetArt() const
{
   return fArt;
}

inline void AMswView::SetArt(DWORD art)
{
   fArt = art;
}

inline AMswDoc* AMswView::GetDocument()
{
   return (AMswDoc*)m_pDocument;
}
