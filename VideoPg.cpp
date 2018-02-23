// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "videopg.h"

AVideoPage::AVideoPage() : CPropertyPage(AVideoPage::IDD),
   fColor(FALSE),
   fInverseVideo(FALSE),
   fScriptDividers(FALSE),
   fShowSpeed(FALSE),
   fShowScriptPos(FALSE),
   fShowTimer(FALSE),
   fLoop(FALSE),
   fLink(FALSE),
   fRightToLeft(FALSE),
   fR2L(NULL),
   fL2R(NULL)
{
	//{{AFX_DATA_INIT(AVideoPage)
	//}}AFX_DATA_INIT
}


void AVideoPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(AVideoPage)
   DDX_Check(pDX, rCtlVcColor, fColor);
   DDX_Check(pDX, rCtlVcInverse, fInverseVideo);
   DDX_Check(pDX, rCtlVcShowDividers, fScriptDividers);
   DDX_Check(pDX, rCtlVcShowSpeed, fShowSpeed);
   DDX_Check(pDX, rCtlVcShowPos, fShowScriptPos);
   DDX_Check(pDX, rCtlVcShowTimer, fShowTimer);
   DDX_Check(pDX, rCtlVcLoop, fLoop);
   DDX_Check(pDX, rCtlVcLink, fLink);
   DDX_Check(pDX, rCtlVcMirror, fRightToLeft);
   DDX_Control(pDX, rCtlVcMirrorIcon, fMirror);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(AVideoPage, CPropertyPage)
	//{{AFX_MSG_MAP(AVideoPage)
   ON_BN_CLICKED(rCtlVcMirror, &AVideoPage::OnMirrorChanged)
	//}}AFX_MSG_MAP
   ON_WM_DESTROY()
END_MESSAGE_MAP()

void AVideoPage::OnMirrorChanged()
{
   this->UpdateData();
   fMirror.SetIcon(fRightToLeft ? fR2L : fL2R);
}

BOOL AVideoPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   fR2L = (HICON)::LoadImage(::AfxGetInstanceHandle(), MAKEINTRESOURCE(rIcoRightToLeft), IMAGE_ICON, 0, 0, 0);
   fL2R = (HICON)::LoadImage(::AfxGetInstanceHandle(), MAKEINTRESOURCE(rIcoLeftToRight), IMAGE_ICON, 0, 0, 0);
   this->OnMirrorChanged();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void AVideoPage::OnDestroy()
{
   VERIFY(::DestroyIcon(fR2L));
   VERIFY(::DestroyIcon(fL2R));

   CPropertyPage::OnDestroy();
}
