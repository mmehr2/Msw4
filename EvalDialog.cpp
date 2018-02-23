// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "EvalDialog.h"


IMPLEMENT_DYNAMIC(AEvalDialog, CDialog)

AEvalDialog::AEvalDialog(CWnd* pParent /*=NULL*/) : 
   CDialog(AEvalDialog::IDD, pParent)
{
}

void AEvalDialog::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, rCtlCompany, fCompanyLogo);
   DDX_Text(pDX, rCtlWarning, fNotice);
}


BEGIN_MESSAGE_MAP(AEvalDialog, CDialog)
END_MESSAGE_MAP()

BOOL AEvalDialog::OnInitDialog()
{
   CDialog::OnInitDialog();

   fBitmap.Attach(::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(rBmpMswLogo),
    IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
   fCompanyLogo.SetBitmap(fBitmap);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}
