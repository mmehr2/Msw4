// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class AFontSizeDlg : public CDialog
{
	DECLARE_DYNAMIC(AFontSizeDlg)

public:
	AFontSizeDlg(CWnd* pParent = NULL);   // standard constructor

	enum { IDD = rDlgOtherSize };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   int fSize;
};
