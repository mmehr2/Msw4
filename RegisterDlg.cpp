// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"

#include "RegisterDlg.h"
#include "Strings.h"
#include "Msw.h"


IMPLEMENT_DYNAMIC(ARegisterDlg, CDialog)
ARegisterDlg::ARegisterDlg(LPCTSTR name, LPCTSTR number, CWnd* pParent /*=NULL*/)
   : CDialog(ARegisterDlg::IDD, pParent)
{
   fComputerId = theApp.fReg.GetComputerId();
   fReg.SetInfo(name, number);
}

void ARegisterDlg::DoDataExchange(CDataExchange* pDX)
{
   CString name     = fReg.GetName();
   CString number   = fReg.GetNumber();

   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_EDIT1, name);
   DDX_Text(pDX, IDC_EDIT2, number);
   DDX_Text(pDX, IDC_COMPUTER_ID, fComputerId);
   DDX_Control(pDX, IDC_BUTTON1, fBtnBuyNow);
   if (pDX->m_bSaveAndValidate)
      fReg.SetInfo(name, number);
}

BEGIN_MESSAGE_MAP(ARegisterDlg, CDialog)
   ON_BN_CLICKED(IDC_BUTTON1, OnPurchase)
END_MESSAGE_MAP()


void ARegisterDlg::OnOK()
{
   this->UpdateData(TRUE);

   if (fReg.GetName().IsEmpty())
      ::AfxMessageBox(rStrRegNameEmpty, MB_OK | MB_ICONHAND);
   else
   {
      UINT uIDReason = 0;	// IDP of reason number is not valid
      if (!fReg.IsValid(uIDReason))
      {
         switch (uIDReason)
         {
            case ARegistration::kEmpty:          uIDReason = rStrEmptyReg; break;
            case ARegistration::kExpired:        uIDReason = rStrRegExpired; break;
            case ARegistration::kPriorVersion:   uIDReason = rStrRegPriorVersion; break;

            case ARegistration::kInvalid:
            default:                            uIDReason = rStrInvalidReg; break;
         }
         AfxMessageBox(uIDReason, MB_OK | MB_ICONHAND);
      }
      else
      {  // This is only considered a "registered Version" if the years is greater than 0
         //if (fReg.GetRegistrationNumberYears() > 0)
         //   g_bRegisteredVersion = TRUE;

         CDialog::OnOK();
      }
   }
}

void ARegisterDlg::OnPurchase()
{
   ::ShellExecute(NULL, _T("open"), gStrBuyUrl, NULL, NULL, SW_SHOWNORMAL);
}
