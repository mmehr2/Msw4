// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include <AfxMt.h>


class ARichEditScrollCtrl : public CRichEditCtrl
{
private:
   DECLARE_DYNAMIC(ARichEditScrollCtrl)

public:
	ARichEditScrollCtrl();

	static DWORD CALLBACK MemFileInCallBack(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);
   void Scroll(int hAmount);
   int GetScrollOffset() const;
   void SetScrollOffset(int offset, int delta=0);
   int GetHeight();

   void Initialize();
   void Uninitialize();

   afx_msg void OnPaint();
   afx_msg BOOL OnEraseBkgnd(CDC*) {return TRUE;}
   afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

private:
   int PaintPiece(int top, int bottom, CRect& update, CDC& dc, COLORREF erase);
   void Draw(CRect& update, CDC& dc);

   int fScrollOffset;
   CSize fSizeBuffer;
	CBitmap fBackBuffer;
   CDC fMemDc;
   CSize fLogPix;
   CRect fTextRect;
   FORMATRANGE fRange;
   int fHeight;
   CHARRANGE fRenderedRange;

   struct Cache
   {
   public:
      Cache();
      void Initialize();
      void Uninitialize();
      void Allocate(FORMATRANGE& range, const CSize& windowSize);
      void Update(const CHARRANGE& cacheRange);

      CCriticalSection fLock;

   private:
      // don't allow copying
      Cache(const Cache& ref);
      Cache& operator = (const Cache& ref);

      static UINT ThreadFunction(LPVOID param);
      void RenderLoop();

      bool fQuit;
      CWinThread* fThread;
   	CBitmap fBackBuffer;
      CDC fMemDc;
      FORMATRANGE fRange;
      CHARRANGE fRangeNeeded;
   } fCache;
};

//-----------------------------------------------------------------------------
// inline functions
inline int ARichEditScrollCtrl::GetScrollOffset() const
{
   return fScrollOffset;
}

inline int ARichEditScrollCtrl::GetHeight()
{
   if (0 == fHeight)
      fHeight = this->PosFromChar(this->GetTextLength()).y;
   return fHeight;
}
