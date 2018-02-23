// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "Msw.h"
#include "RichEditScrollCtrl.h"
#include "Timer.h"


class CAutoLock : CSingleLock
{
public:
   CAutoLock(CSyncObject& o) : CSingleLock(&o) {
      this->Lock();
      ASSERT(this->IsLocked());
   }
};

IMPLEMENT_DYNAMIC(ARichEditScrollCtrl, CRichEditCtrl)
ARichEditScrollCtrl::ARichEditScrollCtrl() :
   fSizeBuffer(0,0),
   fScrollOffset(0XFFFFFFFF),
   fHeight(0)
{
}


BEGIN_MESSAGE_MAP(ARichEditScrollCtrl, CRichEditCtrl)
   ON_WM_PAINT()
   ON_WM_ERASEBKGND()
   ON_WM_SIZE()
   ON_WM_CREATE()
   ON_WM_DESTROY()
END_MESSAGE_MAP()

void ARichEditScrollCtrl::Scroll(int delta)
{
   if (0 != delta)
   {
      CRect update;
      this->ScrollWindowEx(0, delta, NULL, NULL, NULL, update, 0);
      this->SetScrollOffset(fScrollOffset, delta);

      CClientDC dc(this);
      this->Draw(update, dc);
   }
}

void ARichEditScrollCtrl::SetScrollOffset(int offset, int delta)
{
   if (fScrollOffset == (offset - delta))
      return;

   fScrollOffset = offset - delta;
   if (theApp.fLoop)
   {  // if we're above or below the script, loop
      const int height = this->GetHeight();
      if (fScrollOffset < 0)
         fScrollOffset += height;
      else if (fScrollOffset > height)
         fScrollOffset -= height;

      ASSERT(fScrollOffset >= 0);
      ASSERT(fScrollOffset <= height);
   }
}

void ARichEditScrollCtrl::OnPaint()
{
   CRect update;
   if (this->GetUpdateRect(update))
   {
      CPaintDC dc(this);
      this->Draw(update, dc);
   }
}

void ARichEditScrollCtrl::Draw(CRect& update, CDC& dc)
{
   if (update.IsRectEmpty())
      return;

   const bool createBitmap = (NULL == fBackBuffer.GetSafeHandle());
   if (createBitmap)
   {  // create our memory DC and bitmap
      ASSERT(NULL == fMemDc.GetSafeHdc());
	   VERIFY(fMemDc.CreateCompatibleDC(&dc));
   	VERIFY(fBackBuffer.CreateCompatibleBitmap(&dc, fSizeBuffer.cx, fSizeBuffer.cy * 2));

      this->GetRect(fTextRect);

      ::ZeroMemory(&fRange, sizeof(fRange));
      fRange.hdc = fRange.hdcTarget = fMemDc;
      fRange.rc.left = ::MulDiv(fTextRect.left, 1440, fLogPix.cx);
      fRange.rc.right = fRange.rcPage.right = ::MulDiv(fTextRect.right, 1440, fLogPix.cx);
      fRange.rcPage.bottom = ::MulDiv(fTextRect.Height(), 2880, fLogPix.cy);

      fCache.Allocate(fRange, fSizeBuffer);
   }

 	CBitmap* pOldBmp = fMemDc.SelectObject(&fBackBuffer);

   const int height = this->GetHeight();
   long top = update.top + fScrollOffset;
   long bottom = update.bottom + fScrollOffset;
   while ((top > height) && theApp.fLoop)
   {
      top -= height;
      bottom -= height;
   }

   const COLORREF erase = (theApp.fInverseOut) ? 0 : RGB(255,255,255);
   update.top += this->PaintPiece(top, bottom, update, dc, erase);
   while (theApp.fLoop && (update.Height() > 0))
      update.top += this->PaintPiece(0, height, update, dc, erase);

	fMemDc.SelectObject(pOldBmp);
}

int ARichEditScrollCtrl::PaintPiece(int top, int bottom, CRect& update, 
   CDC& dc, COLORREF erase)
{
//ATimer clock;
//clock.Start();

   const int height = this->GetHeight();
   long lineDelta = 0;
   if ((top <= height) && (bottom >= 0))
   {  // draw a portion of the script to the screen
      //TRACE2("top: %d, bottom: %d\n", top, bottom);
      FORMATRANGE range = fRange;
      range.chrg.cpMin = this->CharFromPos(CPoint(fTextRect.left, top));
      range.chrg.cpMax = this->CharFromPos(CPoint(fTextRect.right, bottom));
      const long topLine = this->LineFromChar(range.chrg.cpMin);
      const long bottomLine = this->LineFromChar(range.chrg.cpMax);
      lineDelta = top - this->PosFromChar(range.chrg.cpMin).y;
      range.rc.bottom = ::MulDiv(lineDelta + update.Height(), 2880, fLogPix.cy);
      //TRACE2("top line: %d, bottom line: %d\n", 
      //   this->LineFromChar(range.chrg.cpMin), this->LineFromChar(range.chrg.cpMax));

      if ((range.chrg.cpMin != fRenderedRange.cpMin) || 
          (range.chrg.cpMax > fRenderedRange.cpMax))
      {  // rendering a new portion of the document
         const bool scrollingDown = (range.chrg.cpMax < fRenderedRange.cpMin);

         TRACE2("render %d - %d\n", range.chrg.cpMin, range.chrg.cpMax);
         fMemDc.FillSolidRect(CRect(0, lineDelta, fSizeBuffer.cx, 
            lineDelta + update.Height()), erase);

         this->FormatRange(&range, TRUE);
         fRenderedRange = range.chrg;

         CHARRANGE cacheRange = {0};
         cacheRange.cpMin = this->LineIndex(scrollingDown ? topLine-1 : topLine+1);
         cacheRange.cpMax = this->LineIndex(scrollingDown ? bottomLine-1 : bottomLine+1);
         fCache.Update(cacheRange);
      }

      // we're returning the height rendered, so make sure 
      // the top is 0, as we're assuming
      ASSERT(0 == ::MulDiv(range.rc.top, fLogPix.cy, 1440));
      bottom = ::MulDiv(range.rc.bottom, fLogPix.cy, 1440) - lineDelta;

      if (theApp.fMirror)
      {  // draw mirrored text
         fMemDc.StretchBlt(fTextRect.left, lineDelta, fTextRect.Width(), 
            update.Height(), &fMemDc, fTextRect.right, lineDelta, 
            -fTextRect.Width(), update.Height(), SRCCOPY);
      }
   }
   else
   {
      fMemDc.FillSolidRect(CRect(0,0,fSizeBuffer.cx,update.Height()), erase);
      bottom = update.Height();
   }

//TRACE1("draw %d scan lines\n", update.Height());
   dc.BitBlt(0, update.top, fSizeBuffer.cx, update.Height(), &fMemDc, 0, 
      lineDelta, SRCCOPY);

//TRACE1("draw piece: %d\n", clock.Elapsed());

   return bottom;
}

void ARichEditScrollCtrl::OnSize(UINT nType, int cx, int cy)
{
   CRichEditCtrl::OnSize(nType, cx, cy);

   CSize newSize(cx, cy);
   if (newSize != fSizeBuffer)
   {
   	fSizeBuffer = newSize;
      if (NULL != fBackBuffer.GetSafeHandle())
      {  // free the old buffer
      	fBackBuffer.DeleteObject();
         fBackBuffer.Detach();
      }

      CClientDC dc(this);
      fLogPix.cx = dc.GetDeviceCaps(LOGPIXELSX);
      fLogPix.cy = dc.GetDeviceCaps(LOGPIXELSY);

      fHeight = this->PosFromChar(this->GetTextLength()).y;
   }
}

void ARichEditScrollCtrl::Initialize()
{
   fRenderedRange.cpMin = fRenderedRange.cpMax = 0;
   fCache.Initialize();
}

void ARichEditScrollCtrl::Uninitialize()
{  // destroy the rendering thread
   fCache.Uninitialize();
}

ARichEditScrollCtrl::Cache::Cache() : 
   fThread(0),
   fQuit(true)
{
}

void ARichEditScrollCtrl::Cache::Initialize()
{
   fQuit = false;

   ::ZeroMemory(&fRange, sizeof(fRange));
   ::ZeroMemory(&fRangeNeeded, sizeof(fRangeNeeded));

   // create a thread and enter the rendering loop
   ASSERT(NULL == fThread);   // forget to call Uninitialize?
   fThread = ::AfxBeginThread(ThreadFunction, this, THREAD_PRIORITY_BELOW_NORMAL);
   ASSERT_VALID(fThread);
}

void ARichEditScrollCtrl::Cache::Uninitialize()
{  // terminate the thread
   fQuit = true;
   ::WaitForSingleObject(fThread->m_hThread, INFINITE);
   fThread = NULL;
}

void ARichEditScrollCtrl::Cache::Allocate(FORMATRANGE& range, const CSize& windowSize)
{  // create a memory DC and bitmap for caching
   CAutoLock lock(fLock);
   ASSERT(NULL == fMemDc.GetSafeHdc());
   VERIFY(fMemDc.Attach(::CreateCompatibleDC(range.hdc)));
   VERIFY(fBackBuffer.Attach(::CreateCompatibleBitmap(range.hdc, windowSize.cx, windowSize.cy)));

   fRange = range;
   fRange.hdc = fRange.hdcTarget = fMemDc;
}

void ARichEditScrollCtrl::Cache::Update(const CHARRANGE& cacheRange)
{
   CAutoLock lock(fLock);
   fRangeNeeded = cacheRange;
}

UINT ARichEditScrollCtrl::Cache::ThreadFunction(LPVOID param)
{
   reinterpret_cast<Cache*>(param)->RenderLoop();
   return 0;
}

void ARichEditScrollCtrl::Cache::RenderLoop()
{
   while (!fQuit)
   {
      if ((fRange.chrg.cpMin != fRangeNeeded.cpMin) || 
          (fRange.chrg.cpMax != fRangeNeeded.cpMax))
      {
         fRange.chrg = fRangeNeeded;
         TRACE2("cache %d - %d\n", fRange.chrg.cpMin, fRange.chrg.cpMax);
//         fCtrl->FormatRange(&fRange, TRUE);
      }
   }
}
