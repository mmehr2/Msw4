// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "Msw.h"
#include "AddSlaveDlg.h"


IMPLEMENT_DYNAMIC(AAddSlaveDlg, CDialog)

AAddSlaveDlg::AAddSlaveDlg(CWnd* pParent /*=NULL*/) :
   CDialog(AAddSlaveDlg::IDD, pParent)
{
}

void AAddSlaveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, rCtlRegNumber, fInstallation);
   DDX_Text(pDX, rCtlRegNumber, fUsername);

   if (pDX->m_bSaveAndValidate)
   {
      fInstallation.Trim();
      fUsername.Trim();
      if (fUsername.IsEmpty() || fInstallation.IsEmpty())
      {
         ::AfxMessageBox(rStrAddSlave, MB_ICONEXCLAMATION);
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(AAddSlaveDlg, CDialog)
END_MESSAGE_MAP()
