// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "Msw.h"
#include "RichEditScrollCtrl.h"

IMPLEMENT_DYNAMIC(ARichEditScrollCtrl, CRichEditCtrl)
ARichEditScrollCtrl::ARichEditScrollCtrl() :
   fCx(0),
   fCy(0),
   fScrollOffset(0XFFFFFFFF),
   fHeight(0),
   m_bCallbackSet(FALSE),
   m_pIRichEditOleCallback(NULL),
   fEraseColor((theApp.fInverseOut) ? 0 : RGB(255,255,255))
{
}

ARichEditScrollCtrl::~ARichEditScrollCtrl()
{
   TRACE1("~ARichEditScrollCtrl::m_dwRef = %d\n", this->m_dwRef);
}

int ARichEditScrollCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   if (CRichEditCtrl::OnCreate(lpCreateStruct) == -1)
      return -1;

   // m_pIRichEditOleCallback should have been created in PreSubclassWindow
   ASSERT(m_pIRichEditOleCallback != NULL);

   // set the IExRichEditOleCallback pointer if it wasn't set 
   // successfully in PreSubclassWindow
   if ( !m_bCallbackSet )
      this->SetOLECallback(m_pIRichEditOleCallback);

   return 0;
}

void ARichEditScrollCtrl::PreSubclassWindow() 
{  // base class first
   CRichEditCtrl::PreSubclassWindow();	

   m_pIRichEditOleCallback = new XRichEditOleCallback(true);
   ASSERT(m_pIRichEditOleCallback != NULL);

   m_bCallbackSet = this->SetOLECallback(m_pIRichEditOleCallback);
}

BEGIN_MESSAGE_MAP(ARichEditScrollCtrl, CRichEditCtrl)
   ON_WM_PAINT()
   ON_WM_ERASEBKGND()
   ON_WM_SIZE()
   ON_WM_CREATE()
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
   {  // create our memory DC, bitmap, and background brush
      VERIFY(fMemDc.CreateCompatibleDC(&dc));
   	VERIFY(fBackBuffer.CreateCompatibleBitmap(&dc, fCx, fCy * 2));

      this->GetRect(fTextRect);

      ::ZeroMemory(&fRange, sizeof(fRange));
      fRange.hdc = fRange.hdcTarget = fMemDc;
      fRange.rc.left = ::MulDiv(fTextRect.left, 1440, fLogPix.cx);
      fRange.rc.right = fRange.rcPage.right = ::MulDiv(fTextRect.right, 1440, fLogPix.cx);
      fRange.rcPage.bottom = ::MulDiv(fTextRect.Height(), 2880, fLogPix.cy);
   }

 	CBitmap* pOldBmp = fMemDc.SelectObject(&fBackBuffer);
   const int height = this->GetHeight();
   // get absolute top and bottom in the document
   long top = update.top + fScrollOffset;
   long bottom = update.bottom + fScrollOffset;
   while ((top > height) && theApp.fLoop)
   {  // the end of the script is visible, and we're looping, so
      // loop around to the beginning
      top -= height;
      bottom -= height;
   }

   update.top += this->PaintPiece(top, bottom, update, dc);
   while (theApp.fLoop && (update.Height() > 0))
      update.top += this->PaintPiece(0, height, update, dc);

	fMemDc.SelectObject(pOldBmp);
}

int ARichEditScrollCtrl::PaintPiece(int top, int bottom, CRect& update, CDC& dc)
{
   const int height = this->GetHeight();
   long lineDelta = 0;
   if ((top <= height) && (bottom >= 0))
   {  // draw a portion of the script to the screen
      FORMATRANGE range = fRange;
      const long bottomLine = this->LineFromChar(
         this->CharFromPos(CPoint(fTextRect.left, bottom))) + 1;
      range.chrg.cpMax = this->LineIndex(bottomLine);
      range.chrg.cpMin = this->CharFromPos(CPoint(fTextRect.left, top));
      lineDelta = top - this->PosFromChar(range.chrg.cpMin).y;
      range.rc.bottom = ::MulDiv(lineDelta + update.Height(), 2880, fLogPix.cy);

      fMemDc.FillSolidRect(CRect(0, lineDelta-1, fCx, lineDelta + update.Height() + 2), fEraseColor);
      this->FormatRange(&range, TRUE);

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
      fMemDc.FillSolidRect(CRect(0, 0, fCx, update.Height()), fEraseColor);
      bottom = update.Height();
   }

   dc.BitBlt(0, update.top, fCx, update.Height(), &fMemDc, 0, 
      lineDelta, SRCCOPY);

   return bottom;
}


void ARichEditScrollCtrl::OnSize(UINT nType, int cx, int cy)
{
   CRichEditCtrl::OnSize(nType, cx, cy);
   if ((cx != fCx) || (cy != fCy))
   {
      fCx = cx;
      fCy = cy;

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

ARichEditScrollCtrl::XRichEditOleCallback::XRichEditOleCallback(bool used) :
   fRef(0),
   pStorage(NULL)
{
   if (used) {
      // set up OLE storage
      HRESULT hr = ::StgCreateDocfile(NULL, STGM_TRANSACTED | 
         STGM_READWRITE | STGM_SHARE_EXCLUSIVE | //STGM_DELETEONRELEASE | 
         STGM_CREATE, 0, &pStorage);

      if ((NULL == pStorage) || FAILED(hr))
         ::AfxThrowOleException(hr);
   }
}

HRESULT STDMETHODCALLTYPE 
ARichEditScrollCtrl::XRichEditOleCallback::GetNewStorage(LPSTORAGE* lplpstg)
{
   CString name;
   static int i = 0;
   name.Format(_T("OleStorage%d"), i++);
   HRESULT hr = pStorage->CreateStorage(name, STGM_TRANSACTED | STGM_READWRITE | 
      STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, lplpstg );

   if (!SUCCEEDED(hr))
      ::AfxThrowOleException(hr);

   return hr;
}

HRESULT STDMETHODCALLTYPE ARichEditScrollCtrl::
XRichEditOleCallback::QueryInterface(REFIID iid, void ** ppvObject)
{
   HRESULT hr = E_NOINTERFACE;
   *ppvObject = NULL;

   if ((iid == IID_IUnknown) || (iid == IID_IRichEditOleCallback))
   {
      *ppvObject = this;
      this->AddRef();
      hr = NOERROR;
   }

   return hr;
}

ULONG STDMETHODCALLTYPE ARichEditScrollCtrl::XRichEditOleCallback::AddRef()
{
	return ++fRef;
}

ULONG STDMETHODCALLTYPE ARichEditScrollCtrl::XRichEditOleCallback::Release()
{
   if (0 == --fRef) {
      pStorage->Release();
		delete this;
   }

   return fRef;
}
