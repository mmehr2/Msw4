// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include <memory>


class ADragDropListCtrl : public CListCtrl
{
public:
	ADragDropListCtrl();
	virtual ~ADragDropListCtrl();

   bool IsDragging() const { return (-1 != m_nDropIndex); }

	//{{AFX_VIRTUAL(ADragDropListCtrl)
	//}}AFX_VIRTUAL

   CToolTipCtrl fToolTip;

protected:  // methods
	enum EScrollDirection {scrollNull, scrollUp, scrollDown};

   void DropItem();
	void RestorePrevDropItemState();
	void UpdateSelection(int nDropIndex);
	void SetScrollTimer(EScrollDirection ScrollDirection);
	void KillScrollTimer();
	CImageList* CreateDragImageEx(LPPOINT lpPoint);
	
protected:
   virtual void PreSubclassWindow();
   virtual BOOL PreTranslateMessage(MSG* pMsg);

   //{{AFX_MSG(ADragDropListCtrl)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

protected:  // data
	CDWordArray m_anDragIndexes;
	int m_nDropIndex;
   std::auto_ptr<CImageList> m_pDragImage;
	int m_nPrevDropIndex;
	UINT m_uPrevDropState;
	DWORD m_dwStyle;

	EScrollDirection m_ScrollDirection;
	UINT m_uScrollTimer;
};
