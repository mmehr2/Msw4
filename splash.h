// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class ASplashWnd : public CDialog
{
public:
   ASplashWnd(bool timeout = false);

   BOOL Create(CWnd* pParent) {return CDialog::Create(ASplashWnd::IDD, pParent);}

	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(ASplashWnd)
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT, CPoint) {CDialog::EndDialog(0);}
   afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   enum {
      kTimerId = 1,
      kTimerDuration = 2000   // milliseconds
   };

   //{{AFX_DATA(ASplashWnd)
	enum { IDD = 
#ifdef REGISTER
      rDlgAboutReg
#else
      rDlgAbout
#endif // REGISTER
   };
	CStatic fCompanyLogo;
	CString fVersion;
	//}}AFX_DATA

   CBitmap fBitmap;

   bool fTimeout;
   int fTimer;
   CString fComputerId;
};
