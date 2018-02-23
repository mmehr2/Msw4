// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

//-----------------------------------------------------------------------------
// includes
#include "DragDropListCtrl.h"
#include "resource.h"


class AScriptQueueDlg : public CDialog
{
private:
   DECLARE_DYNAMIC(AScriptQueueDlg)

public:
	AScriptQueueDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~AScriptQueueDlg();

	enum { IDD = rDlgScriptQueue };

   void RestorePosition();
   void RefreshList();
   void UpdateTimes();
   void UpdateNames();

#ifdef _DEBUG
	void CheckSync() const;
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   void WarnIfNotLinking();

   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnDestroy();
   afx_msg void OnNMDoubleClick(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnDropFiles(HDROP hDropInfo);
   afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult); 
   afx_msg void OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnClose();
   afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

   void UpdateStatus();

	DECLARE_MESSAGE_MAP()

private:
   virtual BOOL PreTranslateMessage(MSG* pMsg);

   ADragDropListCtrl fList;
   CImageList fHeaderImages, fListImages;
   CStatic fStatusBar;
   CStatic fStatusBar2;
	HICON fIcon;
   bool fRefreshing;
};
