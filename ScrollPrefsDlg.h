// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class AScrollPrefsDlg : public CPropertyPage
{
private:
   DECLARE_DYNAMIC(AScrollPrefsDlg)

public:
	AScrollPrefsDlg();

	enum { IDD = rDlgScrollPrefs };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   int fLButton;
   int fRButton;
   BOOL fReverseSpeedControl;
public:
   afx_msg void OnCbnSelchangerctlpreflbutton();
};
