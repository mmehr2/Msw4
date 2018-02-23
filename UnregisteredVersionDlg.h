// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "Msw.h"
#include "BuyNowButton.h"
#include "resource.h"
#include "RegGen\Registration.h"


class CUnregisteredVersionDlg : public CDialog
{
private:
   DECLARE_DYNAMIC(CUnregisteredVersionDlg)

public:
	CUnregisteredVersionDlg(LPCTSTR name, LPCTSTR number, CWnd* pParent = NULL);   // standard constructor

	enum {IDD = rDlgUnregisteredVersion};

   ARegistration fReg;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnRegister();
	afx_msg void OnBuyNow();

   void SetCancel(bool cancel) {fCancel = cancel;}

private:
	int fEvaluationDays;
	BOOL fCancel;	// Set to true if Cancel should replace Quit on Cancel button
	ABuyNowButton fBtnBuyNow;
	CFont fFontBold;
	CFont fFontThankYou;
   CString fComputerId;
};
