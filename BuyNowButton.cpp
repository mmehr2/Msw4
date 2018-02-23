// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "BuyNowButton.h"


static const COLORREF kColorText       = RGB(255, 255, 255);
static const COLORREF kColorHighlight  = RGB(0, 0, 255);
static const COLORREF kColorShadow     = RGB(0, 0, 64);
static const COLORREF kColorButton     = RGB(0, 0, 128);

IMPLEMENT_DYNAMIC(ABuyNowButton, CButton)

BEGIN_MESSAGE_MAP(ABuyNowButton, CButton)
END_MESSAGE_MAP()

void ABuyNowButton::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
   // This code only works with buttons.
   ASSERT(lpDIS->CtlType == ODT_BUTTON);

   CDC dc;
	VERIFY(dc.Attach(lpDIS->hDC));
	dc.SaveDC();
	dc.SetBkMode(TRANSPARENT);

   TEXTMETRIC tm;
	VERIFY(dc.GetTextMetrics(&tm));

	if (NULL == fFont.GetSafeHandle())
   {
		LOGFONT lf;
		this->GetFont()->GetLogFont(&lf);
		lf.lfWeight = FW_BOLD;
		VERIFY(fFont.CreateFontIndirect(&lf));
	}

	dc.SetTextColor(kColorText);
	dc.SelectObject(&fFont);
   
	// If drawing selected, add the pushed style to DrawFrameControl.
	dc.FillSolidRect(&lpDIS->rcItem, kColorButton);
	CRect r = lpDIS->rcItem;
   UINT uStyle = DFCS_BUTTONPUSH;
	for (int n = 0; n < 3; ++n)
   {
		if (lpDIS->itemState & ODS_SELECTED)
      {
			uStyle |= DFCS_PUSHED;
			dc.Draw3dRect(r, kColorShadow, kColorHighlight);
		}
      else
			dc.Draw3dRect(r, kColorHighlight, kColorShadow);

      r.DeflateRect(1, 1);
	}

   // Get the button's text.
   CString text;
   this->GetWindowText(text);

   // Draw the button text using the text kColor red.
	dc.DrawText(text, &lpDIS->rcItem, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
	if ((lpDIS->itemState & ODS_FOCUS))
		dc.DrawFocusRect(CRect(lpDIS->rcItem));

   dc.RestoreDC(-1);
	dc.Detach();
}
