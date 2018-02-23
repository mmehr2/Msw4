// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"
#include "afxwin.h"


class AGeneralPrefsDlg : public CPropertyPage
{
private:
   DECLARE_DYNAMIC(AGeneralPrefsDlg)

public:
	AGeneralPrefsDlg();   // standard constructor

	enum { IDD = rDlgGeneralPrefs };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
   afx_msg void OnSetDefaultCharsPerSecond();
   afx_msg void OnSetDefaultFont();
   void SetDefaultFontText();

	DECLARE_MESSAGE_MAP()

	static BOOL CALLBACK AFX_EXPORT EnumFamScreenCallBackEx(
		ENUMLOGFONTEX* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType,
		LPVOID pThis);

public:
   BOOL fClearPaperclips;
   BOOL fResetTimer;
   BOOL fManualTimer;
   double fCharsPerSecond;
   BOOL fInverseEditMode;
   CString fFontLanguage;
   CComboBox fFontLanguageList;
};
