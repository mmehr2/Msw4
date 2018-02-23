// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "GeneralPrefsDlg.h"
#include "Msw.h"


IMPLEMENT_DYNAMIC(AGeneralPrefsDlg, CPropertyPage)

AGeneralPrefsDlg::AGeneralPrefsDlg()
	: CPropertyPage(AGeneralPrefsDlg::IDD)
   , fClearPaperclips(FALSE)
   , fCharsPerSecond(0)
   , fResetTimer(false)
   , fManualTimer(true)
   , fInverseEditMode(false)
   , fFontLanguage(_T(""))
{
}

void AGeneralPrefsDlg::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   DDX_Check(pDX, rCtlPrefClearPc, fClearPaperclips);
   DDX_Check(pDX, rCtlPrefResetTimer, fResetTimer);
   DDX_Check(pDX, rCtlPrefManualTimer, fManualTimer);
   DDX_Check(pDX, rCtlPrefInverseEditMode, fInverseEditMode);
   DDX_Text(pDX, rCtlErtCps, fCharsPerSecond);
   DDV_MinMaxDouble(pDX, fCharsPerSecond, 1.0, 1000.0);
   DDX_CBString(pDX, rCtlPrefsDefFontLang, fFontLanguage);
   DDX_Control(pDX, rCtlPrefsDefFontLang, fFontLanguageList);
}

void AGeneralPrefsDlg::SetDefaultFontText()
{  // set the name of the font button
   this->SetDlgItemText(rCtlButtonDefaultFont, theApp.fDefaultFont.szFaceName);
}

BOOL AGeneralPrefsDlg::OnInitDialog()
{
   const BOOL ret = CPropertyPage::OnInitDialog();
   this->SetDefaultFontText();

   // build font language list
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfCharSet = DEFAULT_CHARSET;
	HDC hDC = theApp.m_dcScreen.m_hDC;
	ASSERT(hDC != NULL);
	::EnumFontFamiliesEx(hDC, &lf, (FONTENUMPROC)EnumFamScreenCallBackEx, (LPARAM)this, NULL);
   fFontLanguageList.SelectString(-1, fFontLanguage);

   return ret;
}

BOOL CALLBACK AFX_EXPORT AGeneralPrefsDlg::EnumFamScreenCallBackEx(ENUMLOGFONTEX* pelf,
	NEWTEXTMETRICEX* /*lpntm*/, int FontType, LPVOID pThis)
{
	if (FontType & RASTER_FONTTYPE)
		return 1;

   AGeneralPrefsDlg* dlg = (AGeneralPrefsDlg*)pThis;
   CComboBox& combo = dlg->fFontLanguageList;
   CString language = pelf->elfScript;
   if (!language.IsEmpty() && (CB_ERR == combo.FindString(-1, language)))
      combo.AddString(language);

   return 1;
}

BEGIN_MESSAGE_MAP(AGeneralPrefsDlg, CPropertyPage)
   ON_BN_CLICKED(rCtlErtDefault, &OnSetDefaultCharsPerSecond)
   ON_BN_CLICKED(rCtlButtonDefaultFont, &OnSetDefaultFont)
END_MESSAGE_MAP()

void AGeneralPrefsDlg::OnSetDefaultCharsPerSecond()
{
   this->UpdateData(TRUE);
   fCharsPerSecond = theApp.kDefaultCharsPerSecond / 10.0;
   this->UpdateData(FALSE);
}

void AGeneralPrefsDlg::OnSetDefaultFont()
{
   // convert to the old format
	CFontDialog dlg(theApp.fDefaultFont);
	if (dlg.DoModal() == IDOK)
   {
      CHARFORMAT oldFormat;
		dlg.GetCharFormat(oldFormat);
      theApp.fDefaultFont = oldFormat;
      this->SetDefaultFontText();
   }
}
