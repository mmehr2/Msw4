// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"

#include "colorlis.h"
#include "resource.h"

int BASED_CODE AColorMenu::indexMap[17] = {
	0,      //black
	1,      //dark red
	2,      //dark green
	3,      //light brown
	4,      //dark blue
	5,      //purple
	6,      //dark cyan
	12,     //gray
	7,      //light gray
	13,     //red
	14,     //green
	15,     //yellow
	16,     //blue
	17,     //magenta
	18,     //cyan
	19,     //white
	0};     //automatic

AColorMenu::AColorMenu()
{
	VERIFY(CreatePopupMenu());
	ASSERT(GetMenuItemCount()==0);
	for (int i=0; i<=16;i++)
		VERIFY(AppendMenu(MF_OWNERDRAW, rCmdColor0+i, (LPCTSTR)(rCmdColor0+i)));
}

COLORREF AColorMenu::GetColor(UINT id)
{
	ASSERT(id >= rCmdColor0);
	ASSERT(id <= rCmdColor16);
	if (id == rCmdColor16) // autocolor
		return ::GetSysColor(COLOR_WINDOWTEXT);
	else
	{
		CPalette* pPal = CPalette::FromHandle( (HPALETTE) GetStockObject(DEFAULT_PALETTE));
		ASSERT(pPal != NULL);
		PALETTEENTRY pe;
		if (pPal->GetPaletteEntries(indexMap[id-rCmdColor0], 1, &pe) == 0)
			return ::GetSysColor(COLOR_WINDOWTEXT);
		else
			return RGB(pe.peRed,pe.peGreen,pe.peBlue);
	}
}

void AColorMenu::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	ASSERT(lpDIS->CtlType == ODT_MENU);
	UINT id = (UINT)(WORD)lpDIS->itemID;
	ASSERT(id == lpDIS->itemData);
	ASSERT(id >= rCmdColor0);
	ASSERT(id <= rCmdColor16);
	CDC dc;
	dc.Attach(lpDIS->hDC);

	CRect rc(lpDIS->rcItem);
	ASSERT(rc.Width() < 500);
	if (lpDIS->itemState & ODS_FOCUS)
		dc.DrawFocusRect(&rc);

	COLORREF cr = (lpDIS->itemState & ODS_SELECTED) ?
		::GetSysColor(COLOR_HIGHLIGHT) :
		dc.GetBkColor();

	CBrush brushFill(cr);
	cr = dc.GetTextColor();

	if (lpDIS->itemState & ODS_SELECTED)
		dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));

	int nBkMode = dc.SetBkMode(TRANSPARENT);
	dc.FillRect(&rc, &brushFill);

	rc.left += 50;
	CString strColor;
	strColor.LoadString(id);
   const int n = strColor.Find(_T('\n'));
   if (n >= 0)
      strColor = strColor.Left(n);
	dc.TextOut(rc.left,rc.top,strColor,strColor.GetLength());
	rc.left -= 45;
	rc.top += 2;
	rc.bottom -= 2;
	rc.right = rc.left + 40;
	CBrush brush(GetColor(id));
	CBrush* pOldBrush = dc.SelectObject(&brush);
	dc.Rectangle(rc);

	dc.SelectObject(pOldBrush);
	dc.SetTextColor(cr);
	dc.SetBkMode(nBkMode);

	dc.Detach();
}

class ADisplayIC : public CDC
{
public:
	ADisplayIC() { CreateIC(_T("DISPLAY"), NULL, NULL, NULL); }
};

void AColorMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	ASSERT(lpMIS->CtlType == ODT_MENU);
	UINT id = (UINT)(WORD)lpMIS->itemID;
	ASSERT(id == lpMIS->itemData);
	ASSERT(id >= rCmdColor0);
	ASSERT(id <= rCmdColor16);

   ADisplayIC dc;
	CString strColor;
	strColor.LoadString(id);
	CSize sizeText = dc.GetTextExtent(strColor,strColor.GetLength());
	ASSERT(sizeText.cx < 500);
	lpMIS->itemWidth = sizeText.cx + 50;
	lpMIS->itemHeight = sizeText.cy;
}
