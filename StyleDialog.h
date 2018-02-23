// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"
#include "style.h"


class CStyleDialog : public CDialog
{
private:
   DECLARE_DYNAMIC(CStyleDialog)

public:
	CStyleDialog(AStyle* style = NULL, CWnd* pParent = NULL);   // standard constructor

	enum { IDD = rDlgStyle };
   AStyle fStyle;
	int m_nIndex;	// Index of existing Style

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	CRect GetFontRect();

public:
	afx_msg void OnFont();
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()
};
