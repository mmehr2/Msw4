// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "GotoLineDlg.h"


IMPLEMENT_DYNAMIC(AGotoLineDlg, CDialog)

AGotoLineDlg::AGotoLineDlg(CWnd* pParent /*=NULL*/) :
   CDialog(AGotoLineDlg::IDD, pParent),
   fLine(-1)
{
}

void AGotoLineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, rCtlGotoLine, fLine);
}

BEGIN_MESSAGE_MAP(AGotoLineDlg, CDialog)
END_MESSAGE_MAP()

BOOL AGotoLineDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // start out with a blank line number
   this->SetDlgItemText(rCtlGotoLine, _T(""));

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}
