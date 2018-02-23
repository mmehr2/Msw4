// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"

#include "ruler.h"
#include "mswview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// the ruler is broken up into two vertical pieces:
// the top shows the page left and right margins that
// will be used for scrolling; the bottom shows the
// margins, indent, and tab stops.
static const BYTE kTopHeight     = 14;
static const BYTE kBottomHeight  = 17;
static const BYTE kHeight        = kTopHeight + kBottomHeight;
static const int kMinScrollWidth = 500;

ARulerItem::ARulerItem(UINT nBitmapID, UINT toolTipId) :
	fBitmap(NULL),
	fBitmapMask(NULL),
	fSize(0,0),
	fYPosPix(INT_MIN),
	fXPosTwips(INT_MIN),
	fAlignment(TA_CENTER),
	fTrack(FALSE),
	fRuler(NULL),
	fDc(NULL),
	fMin(INT_MAX),
   fMax(INT_MIN),
   fToolTipId(toolTipId)
{
	if (nBitmapID != 0)
	{
		fBitmapMask = ::LoadBitmap(
			AfxFindResourceHandle(MAKEINTRESOURCE(nBitmapID+1), RT_BITMAP),
			MAKEINTRESOURCE(nBitmapID+1));
		ASSERT(fBitmapMask != NULL);
		VERIFY(LoadMaskedBitmap(MAKEINTRESOURCE(nBitmapID)));
		BITMAP bm;
		::GetObject(fBitmap, sizeof(BITMAP), &bm);
		fSize = CSize(bm.bmWidth, bm.bmHeight);
	}
}

ARulerItem::~ARulerItem()
{
	if (fBitmap != NULL)
		::DeleteObject(fBitmap);
	if (fBitmapMask != NULL)
		::DeleteObject(fBitmapMask);
}

BOOL ARulerItem::LoadMaskedBitmap(LPCTSTR lpszResourceName)
{
	ASSERT(lpszResourceName != NULL);

	if (fBitmap != NULL)
		::DeleteObject(fBitmap);

	HINSTANCE hInst = AfxFindResourceHandle(lpszResourceName, RT_BITMAP);
	HRSRC hRsrc = ::FindResource(hInst, lpszResourceName, RT_BITMAP);
	if (hRsrc == NULL)
		return FALSE;

	fBitmap = AfxLoadSysColorBitmap(hInst, hRsrc);
	return (fBitmap != NULL);
}

void ARulerItem::SetHorzPosTwips(int nXPos)
{
	if (GetHorzPosTwips() != nXPos)
	{
		if (fTrack)
			DrawFocusLine();
		Invalidate();
		fXPosTwips = nXPos;
		Invalidate();
		if (fTrack)
			DrawFocusLine();
	}
}

void ARulerItem::TrackHorzPosTwips(int nXPos, BOOL /*bOnRuler*/)
{
	int nMin = GetMin();
	int nMax = GetMax();
	if (nXPos < nMin)
		nXPos = nMin;
	if (nXPos > nMax)
		nXPos = nMax;
	SetHorzPosTwips(nXPos);
}

void ARulerItem::DrawFocusLine()
{
	if (GetHorzPosTwips() != 0)
	{
		fTrackRect.left = fTrackRect.right = GetHorzPosPix();
		ASSERT(fDc != NULL);
		int nLeft = fRuler->XRulerToClient(fTrackRect.left);
		fDc->MoveTo(nLeft, fTrackRect.top);
		fDc->LineTo(nLeft, fTrackRect.bottom);
	}
}

void ARulerItem::SetTrack(BOOL b)
{
	fTrack = b;

	if (fDc != NULL) // just in case we lost focus Capture somewhere
	{
		DrawFocusLine();
		fDc->RestoreDC(-1);
		delete fDc ;
		fDc = NULL;
	}
	if (fTrack)
	{
		AMswView* pView = (AMswView*)fRuler->GetView();
		ASSERT(pView != NULL);
		pView->GetClientRect(&fTrackRect);
      fTrackRect.top += kHeight;
		fDc = new CWindowDC(pView);
		fDc->SaveDC();
		fDc->SelectObject(&fRuler->fPenFocusLine);
		fDc->SetROP2(R2_XORPEN);
		DrawFocusLine();
	}
}

void ARulerItem::Invalidate()
{
	CRect rc = GetHitRectPix();
   rc.OffsetRect(0, kTopHeight);
	fRuler->RulerToClient(rc);
	fRuler->InvalidateRect(rc);
}

CRect ARulerItem::GetHitRectPix()
{
	int nx = GetHorzPosPix();
	return CRect(CPoint((fAlignment == TA_CENTER) ? (nx - fSize.cx/2) : 
      (fAlignment == TA_LEFT) ? nx : nx - fSize.cx, fYPosPix), fSize);
}

void ARulerItem::Draw(CDC& dc)
{
	CDC dcBitmap;
	dcBitmap.CreateCompatibleDC(&dc);
	CPoint pt(GetHorzPosPix(), GetVertPosPix());

	HGDIOBJ hbm = ::SelectObject(dcBitmap.m_hDC, fBitmapMask);

	// do mask part
	if (fAlignment == TA_CENTER)
		dc.BitBlt(pt.x - fSize.cx/2, pt.y, fSize.cx, fSize.cy, &dcBitmap, 0, 0, SRCAND);
	else if (fAlignment == TA_LEFT)
		dc.BitBlt(pt.x, pt.y, fSize.cx, fSize.cy, &dcBitmap, 0, 0, SRCAND);
	else // TA_RIGHT
		dc.BitBlt(pt.x - fSize.cx, pt.y, fSize.cx, fSize.cy, &dcBitmap, 0, 0, SRCAND);

	// do image part
	::SelectObject(dcBitmap.m_hDC, fBitmap);

	if (fAlignment == TA_CENTER)
		dc.BitBlt(pt.x - fSize.cx/2, pt.y, fSize.cx, fSize.cy, &dcBitmap, 0, 0, SRCINVERT);
	else if (fAlignment == TA_LEFT)
		dc.BitBlt(pt.x, pt.y, fSize.cx, fSize.cy, &dcBitmap, 0, 0, SRCINVERT);
	else // TA_RIGHT
		dc.BitBlt(pt.x - fSize.cx, pt.y, fSize.cx, fSize.cy, &dcBitmap, 0, 0, SRCINVERT);

	::SelectObject(dcBitmap.m_hDC, hbm);
}

AComboRulerItem::AComboRulerItem(UINT nBitmapID1, UINT nBitmapID2, ARulerItem& item, 
   UINT toolTipId1, UINT toolTipId2) : 
   ARulerItem(nBitmapID1, toolTipId1), 
   fSecondary(nBitmapID2, toolTipId2),
   fLink(item)
{
	fHitPrimary = TRUE;
}

CRect AComboRulerItem::GetHitRectPix()
{
   return fHitPrimary ? ARulerItem::GetHitRectPix() : fSecondary.GetHitRectPix();
}

BOOL AComboRulerItem::HitTestPix(CPoint pt)
{
   fHitPrimary = FALSE;
	if (ARulerItem::GetHitRectPix().PtInRect(pt))
   {
		fHitPrimary = TRUE;
      return TRUE;
   }
	else
		return fSecondary.HitTestPix(pt);
}

void AComboRulerItem::Draw(CDC& dc)
{
	ARulerItem::Draw(dc);
	fSecondary.Draw(dc);
}

void AComboRulerItem::SetHorzPosTwips(int nXPos)
{
	if (fHitPrimary) // only change linked items by delta
		fLink.SetHorzPosTwips(fLink.GetHorzPosTwips() + nXPos - GetHorzPosTwips());
	ARulerItem::SetHorzPosTwips(nXPos);
	fSecondary.SetHorzPosTwips(nXPos);
}

void AComboRulerItem::TrackHorzPosTwips(int nXPos, BOOL /*bOnRuler*/)
{
	int nMin = GetMin();
	int nMax = GetMax();
	if (nXPos < nMin)
		nXPos = nMin;
	if (nXPos > nMax)
		nXPos = nMax;
	SetHorzPosTwips(nXPos);
}

void AComboRulerItem::SetVertPos(int nYPos)
{
	fSecondary.SetVertPos(nYPos);
	nYPos += fSecondary.GetHitRectPix().Height();
	ARulerItem::SetVertPos(nYPos);
}

void AComboRulerItem::SetAlignment(int nAlign)
{
	ARulerItem::SetAlignment(nAlign);
	fSecondary.SetAlignment(nAlign);
}

void AComboRulerItem::SetRuler(ARulerBar* pRuler)
{
	fRuler = pRuler;
	fSecondary.SetRuler(pRuler);
}

void AComboRulerItem::SetBounds(int nMin, int nMax)
{
	ARulerItem::SetBounds(nMin, nMax);
	fSecondary.SetBounds(nMin, nMax);
}

int AComboRulerItem::GetMin() const
{
	if (fHitPrimary)
	{
		int nPDist = GetHorzPosTwips() - ARulerItem::GetMin();
		int nLDist = fLink.GetHorzPosTwips() - fLink.GetMin();
		return GetHorzPosTwips() - min(nPDist, nLDist);
	}
	else
		return ARulerItem::GetMin();
}

int AComboRulerItem::GetMax() const
{
	if (fHitPrimary)
	{
		int nPDist = ARulerItem::GetMax() - GetHorzPosTwips();
		int nLDist = fLink.GetMax() - fLink.GetHorzPosTwips();
		int nMinDist = (nPDist < nLDist) ? nPDist : nLDist;
		return GetHorzPosTwips() + nMinDist;
	}
	else
		return ARulerItem::GetMax();
}

void ATabRulerItem::TrackHorzPosTwips(int nXPos, BOOL bOnRuler)
{
	if (bOnRuler)
		ARulerItem::TrackHorzPosTwips(nXPos, bOnRuler);
	else
		ARulerItem::TrackHorzPosTwips(0, bOnRuler);
}


BEGIN_MESSAGE_MAP(ARulerBar, CControlBar)
	//{{AFX_MSG_MAP(ARulerBar)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_SHOWWINDOW()
	ON_WM_WINDOWPOSCHANGED()
	ON_MESSAGE(WM_SIZEPARENT, OnSizeParent)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

ARulerBar::ARulerBar(BOOL b3DExt) :
	fLeftMargin(IDB_RULER_BLOCK, IDB_RULER_UP, fIndent, rStrRulerLeft, rStrRulerHang),
	fRightMargin(IDB_RULER_UP, rStrRulerRight),
	fScrollLeftMargin(rBmpLeftArrow, rStrRulerLeftScroll),
	fScrollRightMargin(rBmpRightArrow, rStrRulerRightScroll),
	fIndent(IDB_RULER_DOWN, rStrRulerFirstLine),
	fTabItem(IDB_RULER_TAB, rStrRulerTabs)
{
	fDeferInProgress = FALSE;
	fLeftMargin.SetRuler(this);
	fRightMargin.SetRuler(this);
	fIndent.SetRuler(this);

	fScrollLeftMargin.SetRuler(this);
	fScrollRightMargin.SetRuler(this);

	// all of the tab stops share handles
	for (int i=0;i<MAX_TAB_STOPS;i++)
	{
		fTabItems[i].fBitmap = fTabItem.fBitmap;
		fTabItems[i].fBitmapMask = fTabItem.fBitmapMask;
		fTabItems[i].fSize = fTabItem.fSize;
	}

	fUnit.m_nTPU = 0;
	fScroll = 0;

	LOGFONT lf;
	memcpy(&lf, &theApp.m_lf, sizeof(LOGFONT));
	lf.lfHeight = -8;
	lf.lfWidth = 0;
	VERIFY(fFont.CreateFontIndirect(&lf));

	fTabs = 0;
	fLeftMargin.SetVertPos(9);
	fRightMargin.SetVertPos(9);
	fIndent.SetVertPos(-1);
	fScrollLeftMargin.SetVertPos(-kTopHeight - 2);
	fScrollRightMargin.SetVertPos(-kTopHeight - 2);

	m_cxLeftBorder = 0;
	fDraw3DExt = b3DExt;

	m_cyTopBorder = 4;
	m_cyBottomBorder = 6;

	fSelItem = NULL;

	fLogX = theApp.m_dcScreen.GetDeviceCaps(LOGPIXELSX);

	CreateGDIObjects();
}

ARulerBar::~ARulerBar()
{
	// set handles to NULL to avoid deleting twice
	for (int i=0;i<MAX_TAB_STOPS;i++)
	{
		fTabItems[i].fBitmap = NULL;
		fTabItems[i].fBitmapMask = NULL;
	}
}

void ARulerBar::CreateGDIObjects()
{
	fPenFocusLine.DeleteObject();
	fPenBtnHighLight.DeleteObject();
	fPenBtnShadow.DeleteObject();
	fPenWindowFrame.DeleteObject();
	fPenBtnText.DeleteObject();
	fPenBtnFace.DeleteObject();
	fPenWindowText.DeleteObject();
	fPenWindow.DeleteObject();
	fBrushWindow.DeleteObject();
	fBrushBtnFace.DeleteObject();

	fPenFocusLine.CreatePen(PS_DOT, 1,GetSysColor(COLOR_WINDOWTEXT));
	fPenBtnHighLight.CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNHIGHLIGHT));
	fPenBtnShadow.CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
	fPenWindowFrame.CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWFRAME));
	fPenBtnText.CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNTEXT));
	fPenBtnFace.CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNFACE));
	fPenWindowText.CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));
	fPenWindow.CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOW));
	fBrushWindow.CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	fBrushBtnFace.CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
}

void ARulerBar::OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL /*bDisableIfNoHndler*/)
{
	ASSERT_VALID(this);
	//Get the page size and see if changed -- from document
	//get margins and tabs and see if changed -- from view
	AMswView* pView = (AMswView*)GetView();
	if ((NULL != pView) && (fSelItem == NULL)) // only update if not in middle of dragging
	{
		Update(pView->GetPaperSize(), pView->GetMargins());
		Update(pView->GetParaFormatSelection());
		CRect rect;
		pView->GetRichEditCtrl().GetRect(rect);
		CPoint pt = rect.TopLeft();
		pView->ClientToScreen(&pt);
		ScreenToClient(&pt);
		if (m_cxLeftBorder != pt.x)
		{
			m_cxLeftBorder = pt.x;
			Invalidate();
		}
		int nScroll = pView->GetScrollPos(SB_HORZ);
		if (nScroll != fScroll)
		{
			fScroll = nScroll;
			Invalidate();
		}
	}
}

CSize ARulerBar::GetBaseUnits()
{
	ASSERT(fFont.GetSafeHandle() != NULL);
	CFont* pFont = theApp.m_dcScreen.SelectObject(&fFont);
	TEXTMETRIC tm;
	VERIFY(theApp.m_dcScreen.GetTextMetrics(&tm) == TRUE);
	theApp.m_dcScreen.SelectObject(pFont);
	return CSize(tm.tmAveCharWidth, tm.tmHeight);
}

BOOL ARulerBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
	ASSERT_VALID(pParentWnd);   // must have a parent

	dwStyle |= WS_CLIPSIBLINGS;
	// force WS_CLIPSIBLINGS (avoids SetWindowPos bugs)
	m_dwStyle = (dwStyle & CBRS_ALL);

	// create the HWND
	LPCTSTR lpszClass = AfxRegisterWndClass(0, ::LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)(COLOR_BTNFACE+1), NULL);

	CRect rect;
	if (!CWnd::Create(lpszClass, NULL, dwStyle, rect, pParentWnd, nID))
		return FALSE;
	// NOTE: Parent must resize itself for control bar to be resized

	int nMax = 100;
	for (int i = 0; i < MAX_TAB_STOPS; i++)
	{
		fTabItems[i].SetRuler(this);
		fTabItems[i].SetVertPos(8);
		fTabItems[i].SetHorzPosTwips(0);
		fTabItems[i].SetBounds(0, nMax);
	}
	return TRUE;
}

CSize ARulerBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	ASSERT(bHorz);
	CSize size = CControlBar::CalcFixedLayout(bStretch, bHorz);
	CRect rectSize;
	rectSize.SetRectEmpty();
	CalcInsideRect(rectSize, bHorz);       // will be negative size
	size.cy = kHeight - rectSize.Height();
	return size;
}

void ARulerBar::Update(const CParaFormat& pf)
{
	ASSERT(pf.cTabCount <= MAX_TAB_STOPS);

	fLeftMargin.SetHorzPosTwips((int)(pf.dxStartIndent + pf.dxOffset));
	fRightMargin.SetHorzPosTwips(PrintWidth() - (int) pf.dxRightIndent);
	fIndent.SetHorzPosTwips((int)pf.dxStartIndent);

	fScrollLeftMargin.SetHorzPosTwips(0);
	fScrollRightMargin.SetHorzPosTwips(
      XRulerToTwips(theApp.fScrollMarginRight - theApp.fScrollMarginLeft));

	int i = 0;
	for (i = 0; i < pf.cTabCount; i++)
		fTabItems[i].SetHorzPosTwips((int)pf.rgxTabs[i]);
	for (; i < MAX_TAB_STOPS; i++)
		fTabItems[i].SetHorzPosTwips(0);
}

void ARulerBar::Update(CSize sizePaper, const CRect& rectMargins)
{
	if ((sizePaper != fSizePaper) || (rectMargins != fRectMargin))
	{
		fSizePaper = sizePaper;
		fRectMargin = rectMargins;
		Invalidate();
	}
	if (fUnit.m_nTPU != theApp.GetTPU())
	{
		fUnit = theApp.GetUnit();
		Invalidate();
	}
}

void ARulerBar::FillInParaFormat(CParaFormat& pf)
{
	pf.dwMask = PFM_STARTINDENT | PFM_RIGHTINDENT | PFM_OFFSET | PFM_TABSTOPS;
	pf.dxStartIndent = fIndent.GetHorzPosTwips();
	pf.dxOffset = fLeftMargin.GetHorzPosTwips() - pf.dxStartIndent;
	pf.dxRightIndent = PrintWidth() - fRightMargin.GetHorzPosTwips();
	pf.cTabCount = 0L;
	SortTabs();
	for (int i = 0, nPos = 0; i < MAX_TAB_STOPS; i++)
	{
		// get rid of zeroes and multiples
		// i.e. if we have 0,0,0,1,2,3,4,4,5
		// we will get tabs at 1,2,3,4,5
		if (nPos != fTabItems[i].GetHorzPosTwips())
		{
			nPos = fTabItems[i].GetHorzPosTwips();
			pf.rgxTabs[pf.cTabCount++] = nPos;
		}
	}
}

// simple bubble sort is adequate for small number of tabs
void ARulerBar::SortTabs()
{
	for (int i = 0; i < MAX_TAB_STOPS-1; i++)
	{
		for (int j = i + 1; j < MAX_TAB_STOPS; j++)
		{
			if (fTabItems[j].GetHorzPosTwips() < fTabItems[i].GetHorzPosTwips())
			{
				int nPos = fTabItems[j].GetHorzPosTwips();
				fTabItems[j].SetHorzPosTwips(fTabItems[i].GetHorzPosTwips());
				fTabItems[i].SetHorzPosTwips(nPos);
			}
		}
	}
}

void ARulerBar::DoPaint(CDC* pDC)
{
	CControlBar::DoPaint(pDC); // CControlBar::DoPaint -- draws border
	if (fUnit.m_nTPU != 0)
	{
		pDC->SaveDC();
		// offset coordinate system
		CPoint pointOffset(0, kTopHeight);
		RulerToClient(pointOffset);
		pDC->SetViewportOrg(pointOffset);

		DrawFace(*pDC);
		DrawTickMarks(*pDC);

		DrawTabs(*pDC);
		fLeftMargin.Draw(*pDC);
		fRightMargin.Draw(*pDC);
		fIndent.Draw(*pDC);

      // draw scroll margin controls and line between them
      fScrollLeftMargin.Draw(*pDC);
		fScrollRightMargin.Draw(*pDC);
      CRect r;
      r.left = fScrollLeftMargin.GetHorzPosPix() + 
         fScrollLeftMargin.fSize.cx / 2;
      r.right = fScrollRightMargin.GetHorzPosPix() - 
         fScrollRightMargin.fSize.cx / 2;
      r.top = r.bottom = fScrollLeftMargin.GetVertPosPix() + 
         fScrollLeftMargin.fSize.cy / 2;
      r.InflateRect(0, 2);
      pDC->DrawFrameControl(r, DFC_BUTTON, DFCS_BUTTONPUSH);

		pDC->RestoreDC(-1);
	}
	// Do not call CControlBar::OnPaint() for painting messages
}

void ARulerBar::DrawTabs(CDC& dc)
{
	int nPos = 0;
	for (int i = 0; i < MAX_TAB_STOPS; i++)
	{
		if (fTabItems[i].GetHorzPosTwips() > nPos)
			nPos = (fTabItems[i].GetHorzPosTwips());
		fTabItems[i].Draw(dc);
	}
	int nPageWidth = PrintWidth();
	nPos = nPos - nPos%720 + 720;
	dc.SelectObject(&fPenBtnShadow);
	for ( ; nPos < nPageWidth; nPos += 720)
	{
		int nx = XTwipsToRuler(nPos);
		dc.MoveTo(nx, kHeight - 1);
		dc.LineTo(nx, kHeight + 1);
	}
}

void ARulerBar::DrawFace(CDC& dc)
{
	int nPageWidth = XTwipsToRuler(PrintWidth());
	int nPageEdge = XTwipsToRuler(PrintWidth() + fRectMargin.right);

	dc.SaveDC();

	dc.SelectObject(&fPenBtnShadow);
	dc.MoveTo(0,0);
	dc.LineTo(nPageEdge - 1, 0);
	dc.LineTo(nPageEdge - 1, kBottomHeight - 2);
	dc.LineTo(nPageWidth - 1, kBottomHeight - 2);
	dc.LineTo(nPageWidth - 1, 1);
	dc.LineTo(nPageWidth, 1);
	dc.LineTo(nPageWidth, kBottomHeight - 2);

	dc.SelectObject(&fPenBtnHighLight);
	dc.MoveTo(nPageWidth, kBottomHeight - 1);
	dc.LineTo(nPageEdge, kBottomHeight -1);
	dc.MoveTo(nPageWidth + 1, kBottomHeight - 3);
	dc.LineTo(nPageWidth + 1, 1);
	dc.LineTo(nPageEdge - 1, 1);

	dc.SelectObject(&fPenWindow);
	dc.MoveTo(0, kBottomHeight - 1);
	dc.LineTo(nPageWidth, kBottomHeight -1);

	dc.SelectObject(&fPenBtnFace);
	dc.MoveTo(1, kBottomHeight - 2);
	dc.LineTo(nPageWidth - 1, kBottomHeight - 2);

	dc.SelectObject(&fPenWindowFrame);
	dc.MoveTo(0, kBottomHeight - 2);
	dc.LineTo(0, 1);
	dc.LineTo(nPageWidth - 1, 1);

	dc.FillRect(CRect(1, 2, nPageWidth - 1, kBottomHeight-2), &fBrushWindow);
	dc.FillRect(CRect(nPageWidth + 2, 2, nPageEdge - 1, kBottomHeight-2), &fBrushBtnFace);

	CRect rcClient;
	GetClientRect(&rcClient);
	ClientToRuler(rcClient);
	rcClient.top = kBottomHeight;
	rcClient.bottom = kBottomHeight + 2;
	dc.FillRect(rcClient, &fBrushBtnFace);

	CRect rectFill(rcClient.left, kBottomHeight+4, rcClient.right, kBottomHeight+9);
	dc.FillRect(rectFill, &fBrushWindow);

	if (fDraw3DExt)  // draws the 3D extension into the view
	{
		dc.SelectObject(&fPenBtnShadow);
		dc.MoveTo(rcClient.left, kBottomHeight+8);
		dc.LineTo(rcClient.left, kBottomHeight+2);
		dc.LineTo(rcClient.right-1, kBottomHeight+2);

		dc.SelectObject(&fPenWindowFrame);
		dc.MoveTo(rcClient.left+1, kBottomHeight+8);
		dc.LineTo(rcClient.left+1, kBottomHeight+3);
		dc.LineTo(rcClient.right-2, kBottomHeight+3);

		dc.SelectObject(&fPenBtnHighLight);
		dc.MoveTo(rcClient.right-1, kBottomHeight+2);
		dc.LineTo(rcClient.right-1, kBottomHeight+8);

		dc.SelectObject(&fPenBtnFace);
		dc.MoveTo(rcClient.right-2, kBottomHeight+3);
		dc.LineTo(rcClient.right-2, kBottomHeight+8);
	}
	else
	{
		dc.SelectObject(&fPenBtnShadow);
		dc.MoveTo(rcClient.left, kBottomHeight+2);
		dc.LineTo(rcClient.right, kBottomHeight+2);

		dc.SelectObject(&fPenWindowFrame);
		dc.MoveTo(rcClient.left, kBottomHeight+3);
		dc.LineTo(rcClient.right, kBottomHeight+3);
	}

	dc.RestoreDC(-1);
}

void ARulerBar::DrawTickMarks(CDC& dc)
{
	dc.SaveDC();

	dc.SelectObject(&fPenWindowText);
	dc.SelectObject(&fFont);
	dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	dc.SetBkMode(TRANSPARENT);

	DrawDiv(dc, fUnit.m_nSmallDiv, fUnit.m_nLargeDiv, 2);
	DrawDiv(dc, fUnit.m_nMediumDiv, fUnit.m_nLargeDiv, 5);
	DrawNumbers(dc, fUnit.m_nLargeDiv, fUnit.m_nTPU);

	dc.RestoreDC(-1);
}

void ARulerBar::DrawNumbers(CDC& dc, int nInc, int nTPU)
{
	const int nPageWidth = PrintWidth();
	const int nPageEdge = nPageWidth + fRectMargin.right;
	for (int nTwips = nInc; nTwips < nPageEdge; nTwips += nInc)
	{
		if (nTwips == nPageWidth)
			continue;
		const int nPixel = XTwipsToRuler(nTwips);
      CString buf;
      buf.Format(_T("%d"), nTwips/nTPU);
		const CSize sz = dc.GetTextExtent(buf);
		dc.ExtTextOut(nPixel - sz.cx/2, kBottomHeight/2 - sz.cy/2, 0, NULL, buf, NULL);
	}
}

void ARulerBar::DrawDiv(CDC& dc, int nInc, int nLargeDiv, int nLength)
{
	const int nPageWidth = PrintWidth();
	const int nPageEdge = nPageWidth + fRectMargin.right;
	for (int nTwips = nInc; nTwips < nPageEdge; nTwips += nInc)
	{
		if (nTwips == nPageWidth || nTwips%nLargeDiv == 0)
			continue;
      const int nPixel = XTwipsToRuler(nTwips);
		dc.MoveTo(nPixel, kBottomHeight/2 - nLength/2);
		dc.LineTo(nPixel, kBottomHeight/2 - nLength/2 + nLength);
	}
}

ARulerItem* ARulerBar::HitTest(CPoint point)
{
   ARulerItem* item = NULL;
	CPoint pt = point;
	this->ClientToRuler(pt);
   // first check the top portion
   pt.Offset(0, -kTopHeight);
	if (fScrollLeftMargin.HitTestPix(pt))
		item = &fScrollLeftMargin;
	else if (fScrollRightMargin.HitTestPix(pt))
		item = &fScrollRightMargin;

   if (pt.y >= -1)
   {  // and now check the bottom
	   if (fLeftMargin.HitTestPix(pt))
		   item = &fLeftMargin;
	   else if (fRightMargin.HitTestPix(pt))
		   item = &fRightMargin;
	   else if (fIndent.HitTestPix(pt))
		   item = &fIndent;
	   else
		   item = GetHitTabPix(pt);

      if (item == NULL)
		   item = GetFreeTab();
   }

   return item;
}

void ARulerBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (NULL == (fSelItem = this->HitTest(point)))
		return;

	this->SetCapture();

	fSelItem->SetTrack(TRUE);
	this->SetMarginBounds();
	this->OnMouseMove(nFlags, point);
}

void ARulerBar::SetMarginBounds()
{
	fLeftMargin.SetBounds(0, fRightMargin.GetHorzPosTwips());
	fIndent.SetBounds(0, fRightMargin.GetHorzPosTwips());

	int nMin = (fLeftMargin.GetHorzPosTwips() > fIndent.GetHorzPosTwips()) ?
		fLeftMargin.GetHorzPosTwips() : fIndent.GetHorzPosTwips();
	int nMax = PrintWidth() + fRectMargin.right;
	fRightMargin.SetBounds(nMin, nMax);

	// tabs can go from zero to the right page edge
	for (int i = 0; i < MAX_TAB_STOPS; i++)
		fTabItems[i].SetBounds(0, nMax);

	fScrollLeftMargin.SetBounds(
      XRulerToTwips(ARtfHelper::kCueBarWidth + ARtfHelper::kRtfHOffset - m_cxLeftBorder),
      fScrollRightMargin.GetHorzPosTwips() - kMinScrollWidth);

   CRect screen;
   ::SystemParametersInfo(SPI_GETWORKAREA, 0, &screen, 0);
   const int cx = screen.Width() - (ARtfHelper::kCueBarWidth + 
      ARtfHelper::kRtfHOffset + GetSystemMetrics(SM_CXVSCROLL));
   fScrollRightMargin.SetBounds(fScrollLeftMargin.GetHorzPosTwips() + kMinScrollWidth,
      XRulerToTwips(cx));
}

ARulerItem* ARulerBar::GetFreeTab()
{
	for (int i = 0; i < MAX_TAB_STOPS; i++)
		if (fTabItems[i].GetHorzPosTwips() == 0)
			return &fTabItems[i];

   return NULL;
}

ATabRulerItem* ARulerBar::GetHitTabPix(CPoint point)
{
	for (int i = 0; i < MAX_TAB_STOPS; i++)
		if (fTabItems[i].HitTestPix(point))
			return &fTabItems[i];

   return NULL;
}

void ARulerBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (::GetCapture() != m_hWnd)
		return;

   OnMouseMove(nFlags, point);
	fSelItem->SetTrack(FALSE);
	ReleaseCapture();

   if ((fSelItem == &fScrollLeftMargin) ||
       (fSelItem == &fScrollRightMargin))
   {
      int offset = m_cxLeftBorder - ARtfHelper::kCueBarWidth - 
         ARtfHelper::kRtfHOffset;
      theApp.SetScrollMargins(offset + fScrollLeftMargin.GetHorzPosPix(), 
         offset + fScrollRightMargin.GetHorzPosPix());
   }
   else
   {
	   AMswView* pView = (AMswView*)GetView();
	   ASSERT(pView != NULL);
	   CParaFormat pf = pView->GetParaFormatSelection();
	   FillInParaFormat(pf);
	   pView->SetParaFormat(pf);
   }
	fSelItem = NULL;
}

void ARulerBar::OnMouseMove(UINT nFlags, CPoint point)
{
	CControlBar::OnMouseMove(nFlags, point);
// use ::GetCapture to avoid creating temporaries
	if (::GetCapture() != m_hWnd)
		return;

   ASSERT(fSelItem != NULL);
	CRect rc(0,0, XTwipsToRuler(PrintWidth() + fRectMargin.right), kHeight);
	RulerToClient(rc);
	BOOL bOnRuler = rc.PtInRect(point);

// snap to minimum movement
	point.x = XClientToTwips(point.x);
	point.x += fUnit.m_nMinMove/2;
	point.x -= point.x%fUnit.m_nMinMove;

	fSelItem->TrackHorzPosTwips(point.x, bOnRuler);
	UpdateWindow();
}

void ARulerBar::OnSysColorChange()
{
	CControlBar::OnSysColorChange();
	CreateGDIObjects();
	Invalidate();
}

void ARulerBar::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos)
{
	CControlBar::OnWindowPosChanging(lpwndpos);
	CRect rect;
	GetClientRect(rect);
	int minx = min(rect.Width(), lpwndpos->cx);
	int maxx = max(rect.Width(), lpwndpos->cx);
	rect.SetRect(minx-2, rect.bottom - 6, minx, rect.bottom);
	InvalidateRect(rect);
	rect.SetRect(maxx-2, rect.bottom - 6, maxx, rect.bottom);
	InvalidateRect(rect);
}

void ARulerBar::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CControlBar::OnShowWindow(bShow, nStatus);
   fDeferInProgress = FALSE;
}

void ARulerBar::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos)
{
	CControlBar::OnWindowPosChanged(lpwndpos);
	fDeferInProgress = FALSE;
}

LRESULT ARulerBar::OnSizeParent(WPARAM wParam, LPARAM lParam)
{
	BOOL bVis = GetStyle() & WS_VISIBLE;
	if ((bVis && (m_nStateFlags & delayHide)) ||
		(!bVis && (m_nStateFlags & delayShow)))
	{
		fDeferInProgress = TRUE;
	}
	return CControlBar::OnSizeParent(wParam, lParam);
}

INT_PTR ARulerBar::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
   if (NULL != pTI)
   {
      ARulerItem* item = const_cast<ARulerBar*>(this)->HitTest(point);
      if ((NULL != item) && (0 != item->GetToolTipId()))
      {  // get the tool tip information for this item
         CRect r(item->GetHitRectPix());
         r.OffsetRect(0, kTopHeight);
	      const_cast<ARulerBar*>(this)->RulerToClient(r);
         pTI->rect = r;
         pTI->hwnd = m_hWnd;
         pTI->uId = item->GetToolTipId();
         CString text;
         VERIFY(text.LoadString(pTI->uId));
         TCHAR* buffer = new TCHAR[text.GetLength() + 1];
         ::lstrcpy(buffer, text);
         pTI->lpszText = buffer;
         return pTI->uId;
      }
   }

   return CControlBar::OnToolHitTest(point, pTI);
}
