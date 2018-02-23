// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "FindReplaceDlg.h"


IMPLEMENT_DYNAMIC(AFindReplaceDlg, CFindReplaceDialog)

AFindReplaceDlg::AFindReplaceDlg(CWnd* /*pParent = NULL*/)
	: CFindReplaceDialog()
   , fUseFindFormat(0)
   , fUseReplaceFormat(0)
{
}

void AFindReplaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CFindReplaceDialog::DoDataExchange(pDX);
   DDX_Check(pDX, rCtlUseFindFormat, fUseFindFormat);
   DDX_Check(pDX, rCtlUseReplaceFormat, fUseReplaceFormat);
}


BEGIN_MESSAGE_MAP(AFindReplaceDlg, CFindReplaceDialog)
   ON_BN_CLICKED(rCtlUseFindFormat, &OnUseFindFormat)
   ON_BN_CLICKED(rCtlUseReplaceFormat, &OnUseReplaceFormat)
   ON_BN_CLICKED(rCtlFindFormat, &OnFindFormat)
   ON_BN_CLICKED(rCtlReplaceFormat, &OnReplaceFormat)
END_MESSAGE_MAP()

void AFindReplaceDlg::OnUseFindFormat()
{
   this->UpdateData();
   this->GetDlgItem(rCtlFindFormat)->EnableWindow(fUseFindFormat);
}

void AFindReplaceDlg::OnUseReplaceFormat()
{
   this->UpdateData();
   this->GetDlgItem(rCtlReplaceFormat)->EnableWindow(fUseReplaceFormat);
}

void AFindReplaceDlg::OnReplaceFormat()
{  // convert to the old format
   CHARFORMAT oldFormat = fReplaceFormat;
   oldFormat.cbSize = sizeof(oldFormat);
	CFontDialog dlg(fReplaceFormat);
   dlg.m_cf.Flags |= CF_EFFECTS;
   dlg.m_cf.Flags &= ~CF_NOSCRIPTSEL;
	if (dlg.DoModal() == IDOK)
   {
		dlg.GetCharFormat(oldFormat);
      fReplaceFormat = oldFormat;
   }
}
