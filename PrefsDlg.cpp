// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "PrefsDlg.h"
#include "Msw.h"


IMPLEMENT_DYNAMIC(APrefsDlg, CPropertySheet)

APrefsDlg::APrefsDlg(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
   this->AddPage(&fGeneralPage);
   this->AddPage(&fScrollPage);
   this->AddPage(&fVideoPage);
   this->AddPage(&fCaptionsPage);

   // remove the Apply button
   m_psh.dwFlags |= PSH_NOAPPLYNOW;
}

APrefsDlg::APrefsDlg(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

BEGIN_MESSAGE_MAP(APrefsDlg, CPropertySheet)
   ON_BN_CLICKED(IDHELP, OnHelp)
   ON_WM_HELPINFO()
END_MESSAGE_MAP()

void APrefsDlg::OnHelp()
{
   theApp.ShowHelp(HH_HELP_CONTEXT, rDlgGeneralPrefs);
}

BOOL APrefsDlg::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();
   this->ModifyStyleEx(0, WS_EX_CONTEXTHELP);
   return bResult;
}

BOOL APrefsDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
   if (HELPINFO_WINDOW == pHelpInfo->iContextType)
   {
      const DWORD ids[] = {
         rCtlPrefClearPc,           rCtlPrefClearPc, 
         rCtlPrefResetTimer,        rCtlPrefResetTimer,
         rCtlPrefPrintCue,          rCtlPrefPrintCue,
         rCtlPrefInverseEditMode,   rCtlPrefInverseEditMode,
         rCtlErtCps,                rCtlErtCps,
         rCtlErtDefault,            rCtlErtDefault,
         rCtlButtonDefaultFont,     rCtlButtonDefaultFont,
         0}; 
      theApp.ShowWhatsThisHelp(ids, pHelpInfo->hItemHandle);
      return TRUE;
   }

   return CPropertySheet::OnHelpInfo(pHelpInfo);
}
