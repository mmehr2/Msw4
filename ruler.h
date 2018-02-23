// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#pragma once

#include "options.h"

class AWordPadView;
class AWordPadDoc;
class ARulerBar;

// ruler items include left margin, right margin, indent, and tabs

// horz positions in twips -- necessary to avoid rounding errors
// vertical position in pixels
class ARulerItem
{
public:
	ARulerItem(UINT nBitmapID = 0, UINT toolTipId = 0);
	~ARulerItem();

   virtual BOOL HitTestPix(CPoint pt) { return GetHitRectPix().PtInRect(pt); }
	virtual void Draw(CDC& dc);
	virtual void SetHorzPosTwips(int nXPos);
	virtual void TrackHorzPosTwips(int nXPos, BOOL bOnRuler = TRUE);
	virtual void SetVertPos(int nYPos) { fYPosPix = nYPos; }
	virtual void SetAlignment(int nAlign) {fAlignment = nAlign;}
	virtual void SetRuler(ARulerBar* pRuler) {fRuler = pRuler;}
	virtual void SetBounds(int nMin, int nMax) { fMin = nMin; fMax = nMax; }
	int GetMin() const { return fMin;}
	int GetMax() const { return fMax;}
	void Invalidate();
	int GetVertPosPix() const { return fYPosPix;}
	int GetHorzPosTwips() const { return fXPosTwips;}
	int GetHorzPosPix() const;
	virtual CRect GetHitRectPix();
	void DrawFocusLine();
	void SetTrack(BOOL b);
   virtual UINT GetToolTipId() const;

	HBITMAP fBitmap;
	HBITMAP fBitmapMask;
	CSize fSize;   // size of item in pixels

	BOOL LoadMaskedBitmap(LPCTSTR lpszResourceName);

protected:
	int fYPosPix;
	int fXPosTwips;
	int fAlignment;
	BOOL fTrack;
	ARulerBar* fRuler;
	CRect fTrackRect;
	CDC* fDc; // dc used for drawing tracking line
	int fMin, fMax;
   UINT fToolTipId;
};

class AComboRulerItem : public ARulerItem
{
public:
	AComboRulerItem(UINT nBitmapID1, UINT nBitmapID2, ARulerItem& item, 
      UINT toolTipId1, UINT toolTipId2);

   virtual BOOL HitTestPix(CPoint pt);
	virtual void Draw(CDC& dc);
	virtual void SetHorzPosTwips(int nXPos);
	virtual void TrackHorzPosTwips(int nXPos, BOOL bOnRuler = TRUE);
	virtual void SetVertPos(int nYPos);
	virtual void SetAlignment(int nAlign);
	virtual void SetRuler(ARulerBar* pRuler);
	virtual void SetBounds(int nMin, int nMax);
	int GetMin() const;
	int GetMax() const;
   virtual UINT GetToolTipId() const;
	virtual CRect GetHitRectPix();

protected:
	ARulerItem fSecondary;
	ARulerItem& fLink;
	BOOL fHitPrimary;
};

class ATabRulerItem : public ARulerItem
{
public:
	ATabRulerItem() { SetAlignment(TA_LEFT); }

   virtual void Draw(CDC& dc) {if (GetHorzPosTwips() != 0) ARulerItem::Draw(dc);}
	virtual void TrackHorzPosTwips(int nXPos, BOOL bOnRuler = TRUE);
	virtual BOOL HitTestPix(CPoint pt) { return (GetHorzPosTwips() != 0) ? ARulerItem::HitTestPix(pt) : FALSE;}
};

class ARulerBar : public CControlBar
{
public:
	ARulerBar(BOOL b3DExt = TRUE);
	~ARulerBar();

public:
	virtual BOOL Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID);
protected:
	void Update(const CParaFormat& pf);
	void Update(CSize sizePaper, const CRect& rectMargins);

public:
	CSize GetBaseUnits();
	int PrintWidth() {return fSizePaper.cx - fRectMargin.left - fRectMargin.right;}

   AComboRulerItem fLeftMargin;
	ARulerItem fRightMargin;
	ARulerItem fIndent;
	ARulerItem fTabItem;
	ATabRulerItem fTabItems[MAX_TAB_STOPS];
   ARulerItem fScrollLeftMargin;
   ARulerItem fScrollRightMargin;

   BOOL fDeferInProgress;
	BOOL fDraw3DExt;
	AUnit fUnit;
	ARulerItem* fSelItem;
	CFont fFont;
	CSize fSizePaper;
	CRect fRectMargin;
	int fTabs;
	int fLogX;
	int fLinePos;
	int fScroll; // in pixels

	CPen fPenFocusLine;
	CPen fPenBtnHighLight;
	CPen fPenBtnShadow;
	CPen fPenWindowFrame;
	CPen fPenBtnText;
	CPen fPenBtnFace;
	CPen fPenWindowText;
	CPen fPenWindow;
	CBrush fBrushWindow;
	CBrush fBrushBtnFace;

public:
	virtual void DoPaint(CDC* pDC);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	void ClientToRuler(CPoint& pt) {pt.Offset(-m_cxLeftBorder+fScroll, -m_cyTopBorder);}
	void ClientToRuler(CRect& rect) {rect.OffsetRect(-m_cxLeftBorder+fScroll, -m_cyTopBorder);}
	void RulerToClient(CPoint& pt) {pt.Offset(m_cxLeftBorder-fScroll, m_cyTopBorder);}
	void RulerToClient(CRect& rect) {rect.OffsetRect(m_cxLeftBorder-fScroll, m_cyTopBorder);}

	int XTwipsToClient(int nT) const {return MulDiv(nT, fLogX, 1440) + m_cxLeftBorder - fScroll;}
	int XClientToTwips(int nC) const {return MulDiv(nC - m_cxLeftBorder + fScroll, 1440, fLogX);}

	int XTwipsToRuler(int nT) const {return MulDiv(nT, fLogX, 1440);}
	int XRulerToTwips(int nR) const {return MulDiv(nR, 1440, fLogX);}

	int XRulerToClient(int nR) const {return nR + m_cxLeftBorder - fScroll;}
	int XClientToRuler(int nC) const {return nC - m_cxLeftBorder + fScroll;}

protected:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	void CreateGDIObjects();
	void DrawFace(CDC& dc);
	void DrawTickMarks(CDC& dC);
	void DrawNumbers(CDC& dc, int nInc, int nTPU);
	void DrawDiv(CDC& dc, int nInc, int nLargeDiv, int nLength);
	void DrawTabs(CDC& dc);
	void FillInParaFormat(CParaFormat& pf);
	void SortTabs();
	void SetMarginBounds();
	ARulerItem* GetFreeTab();
	CView* GetView()
	{
		ASSERT(GetParent() != NULL);
		return dynamic_cast<CView*>(GetParent());
	}
	CDocument* GetDocument() { return GetView()->GetDocument(); }

	ATabRulerItem* GetHitTabPix(CPoint pt);

	//{{AFX_MSG(ARulerBar)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSysColorChange();
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	//}}AFX_MSG
	afx_msg LRESULT OnSizeParent(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

   ARulerItem* HitTest(CPoint point);
   virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;

	friend class ARulerItem;
};

inline int ARulerItem::GetHorzPosPix() const
{
   return fRuler->XTwipsToRuler(fXPosTwips);
}

inline UINT ARulerItem::GetToolTipId() const
{
   return fToolTipId;
}

inline UINT AComboRulerItem::GetToolTipId() const
{
   if (fHitPrimary)
      return ARulerItem::GetToolTipId();
   else
      return fSecondary.GetToolTipId();
}
