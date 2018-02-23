// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "FontSizeDlg.h"


IMPLEMENT_DYNAMIC(AFontSizeDlg, CDialog)

AFontSizeDlg::AFontSizeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(AFontSizeDlg::IDD, pParent)
   , fSize(0)
{
}

void AFontSizeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, rCtrlOtherSize, fSize);
}


BEGIN_MESSAGE_MAP(AFontSizeDlg, CDialog)
END_MESSAGE_MAP()
