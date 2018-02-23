// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "ScrollPosMarker.h"
#include "ScriptQueue.h"
#include "Msw.h"

AScrollPosIndicator::AScrollPosIndicator() :
   fScale(0),
   fPos(kInvalidPos),
   fLastY(kInvalidPos)
{
   fArea.SetRect(0,0,0,0);
}

void AScrollPosIndicator::Create(const CRect& area, CDC& dc, AScriptQueue& q, int scale)
{
   ASSERT(scale >= 0);

   fArea = area;
   fScale = scale;

   this->Draw(dc, q);
}

void AScrollPosIndicator::Draw(CDC& dc, AScriptQueue& q)
{  // since we're erasing the entire area, make sure 
   // we don't try to erase the last blip in Update.
   fPos = kInvalidPos;
   fLastY = kInvalidPos;

   dc.SaveDC();

   CFont font;
   font.CreateFont(-9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
      ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      DEFAULT_QUALITY, FF_ROMAN, NULL);
   CFont* saveFont = dc.SelectObject(&font);

   dc.SetTextColor(!theApp.fInverseOut ? RGB(159, 159, 159) : RGB(96, 96, 96));
	dc.SetBkMode(TRANSPARENT);   
	dc.SetROP2(R2_NOTCOPYPEN);

   CRect r(fArea);
   CBrush eraser(!theApp.fInverseOut ? kWhite : kBlack);
   CGdiObject* savePen = dc.SelectStockObject(DC_PEN);
   dc.SetDCPenColor(!theApp.fInverseOut ? kWhite : kBlack);
   dc.FillRect(&r, &eraser);
   dc.MoveTo(r.left, r.top+1);
   dc.LineTo(r.left, r.bottom-1);
   dc.LineTo(r.right, r.bottom-1);
   dc.LineTo(r.right, r.top + 5);

   ::InflateRect(&r, -1, -1); 
   const int x = (r.left + r.right) / 2;
   for (size_t i = 0; i < q.size(); ++i)
   {
      const int top = this->PosToPixel(q[i].GetStart());
      const int bottom = this->PosToPixel(q[i].GetEnd());

      // draw rect with 'folded' corner
      dc.MoveTo(r.left, r.top + top);                   
      dc.LineTo(r.right - 4, r.top + top);                   
      dc.LineTo(r.right, r.top + top + 4);
      dc.LineTo(r.right - 4, r.top + top + 4);
      dc.LineTo(r.right - 4, r.top + top);

      // draw the script number
      CString str;
      str.Format(_T("%d"), i + 1);
      const CSize textSize = dc.GetTextExtent(str);
      if ((bottom - top) >= (textSize.cy + 8))
      {
         CRect rText;
         const int center = (top + bottom) / 2;
         rText.top = r.top + center - (textSize.cy / 2) - 1;
         rText.bottom = rText.top + textSize.cy + 1;
         rText.left =  x - (textSize.cx / 2 ) - 1;
         rText.right = rText.left + textSize.cx + 2;
         dc.DrawText(str, rText, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
      }
   }
   dc.SelectObject(saveFont);
   dc.SelectObject(savePen);
   dc.RestoreDC(-1);
}

void AScrollPosIndicator::Update(int pos, CWnd& wnd)
{
   int y = this->PosToPixel(pos);
   if (y != fLastY)
   {
      CClientDC dc(&wnd);
      
      if (fLastY >= 0)  // erase the last blip
         this->DrawBlip(fLastY, dc);
      if (pos >= 0)  // and draw the new one
         this->DrawBlip(y, dc);

      fLastY = y;
      fPos = pos;
   }
}

int AScrollPosIndicator::PosToPixel(int pos)
{
   int pix = 0;
   if (fScale != 0)
   {
      if ((long)pos < 0)
         pos = 0;
      else if (pos >= fScale)
         pos = fScale - 1;

      pix = ((pos * (DWORD)(fArea.Height() - 2)) / fScale);
   }

   return pix;
}

void AScrollPosIndicator::DrawBlip(int y, CDC& dc)
{  // the 'blip' is a horizontal line with a diamond in the center
   const int saveRop = dc.SetROP2(R2_NOTXORPEN);
   const int mid = (fArea.right + fArea.left) / 2;

   ::ShowCursor(FALSE);
   dc.MoveTo(fArea.left, fArea.top + y);                   
   dc.LineTo(mid - 2, fArea.top + y);
   dc.MoveTo(mid + 3, fArea.top + y);                   
   dc.LineTo(fArea.right, fArea.top + y);
   dc.MoveTo(mid - 2, fArea.top + y);
   dc.LineTo(mid, fArea.top + y - 2);
   dc.LineTo(mid + 2, fArea.top + y);
   dc.LineTo(mid, fArea.top + y + 2);
   dc.LineTo(mid - 2, fArea.top + y);
   dc.SetROP2(saveRop);        
   ::ShowCursor(TRUE);
}
