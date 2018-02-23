// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "ScrollPrefsDlg.h"


IMPLEMENT_DYNAMIC(AScrollPrefsDlg, CPropertyPage)

AScrollPrefsDlg::AScrollPrefsDlg()
	: CPropertyPage(AScrollPrefsDlg::IDD)
   , fLButton(-1)
   , fRButton(-1)
   , fReverseSpeedControl(false)
{
}

void AScrollPrefsDlg::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   DDX_CBIndex(pDX, rCtlPrefLButton, fLButton);
   DDX_CBIndex(pDX, rCtlPrefRButton, fRButton);
   DDX_Check(pDX, rCtlPrefRevSpeed, fReverseSpeedControl);
}


BEGIN_MESSAGE_MAP(AScrollPrefsDlg, CPropertyPage)
END_MESSAGE_MAP()
