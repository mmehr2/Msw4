// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"
#include "RegGen/Registration.h"
#include "BuyNowButton.h"

class ARegisterDlg : public CDialog
{
private:
   DECLARE_DYNAMIC(ARegisterDlg)

public:
   ARegisterDlg(LPCTSTR name, LPCTSTR number, CWnd* pParent = NULL);   // standard constructor

   enum { IDD = rDlgRegister };

   ARegistration fReg;

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual void OnOK();
   afx_msg void OnPurchase();

   DECLARE_MESSAGE_MAP()

private:
   ABuyNowButton fBtnBuyNow;
   CString fComputerId;
};
