// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


class ARichEditScrollCtrl : public CRichEditCtrl
{
private:
   DECLARE_DYNAMIC(ARichEditScrollCtrl)

public:
	ARichEditScrollCtrl();
	virtual ~ARichEditScrollCtrl();

   void Scroll(int hAmount);
   int GetScrollOffset() const;
   void SetScrollOffset(int offset, int delta=0);
   int GetHeight();

   afx_msg void OnPaint();
   afx_msg BOOL OnEraseBkgnd(CDC*) {return TRUE;}
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()

private:
   int PaintPiece(int top, int bottom, CRect& update, CDC& dc);
   void Draw(CRect& update, CDC& dc);
   virtual void PreSubclassWindow();

   class XRichEditOleCallback;  // forward declaration (see below in this header file)
   XRichEditOleCallback* m_pIRichEditOleCallback;
   BOOL m_bCallbackSet;

	BEGIN_INTERFACE_PART(RichEditOleCallback, IRichEditOleCallback)
   	XRichEditOleCallback();
		INIT_INTERFACE_PART(CRichEditView, RichEditOleCallback)
		STDMETHOD(GetNewStorage) (LPSTORAGE*);
      STDMETHOD(GetInPlaceContext) (LPOLEINPLACEFRAME*, LPOLEINPLACEUIWINDOW*, LPOLEINPLACEFRAMEINFO) {return S_OK;}
		STDMETHOD(ShowContainerUI) (BOOL) {return S_OK;}
		STDMETHOD(QueryInsertObject) (LPCLSID, LPSTORAGE, LONG) {return S_OK;}
		STDMETHOD(DeleteObject) (LPOLEOBJECT) {return S_OK;}
		STDMETHOD(QueryAcceptData) (LPDATAOBJECT, CLIPFORMAT*, DWORD,BOOL, HGLOBAL) {return S_OK;}
		STDMETHOD(ContextSensitiveHelp) (BOOL) {return S_OK;}
		STDMETHOD(GetClipboardData) (CHARRANGE*, DWORD, LPDATAOBJECT*) {return S_OK;}
		STDMETHOD(GetDragDropEffect) (BOOL, DWORD, LPDWORD) {return S_OK;}
		STDMETHOD(GetContextMenu) (WORD, LPOLEOBJECT, CHARRANGE*, HMENU*) {return S_OK;}

      IStorage* pStorage;
      DWORD fRef;
   END_INTERFACE_PART(RichEditOleCallback)

   CBitmap fBackBuffer;
   CDC fMemDc;
   CSize fLogPix;
   CRect fTextRect;
   FORMATRANGE fRange;
   int fScrollOffset;
   int fCx, fCy; // width of window
   int fHeight;
   const COLORREF fEraseColor;
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
