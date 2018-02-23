// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class AEvalDialog : public CDialog
{
	DECLARE_DYNAMIC(AEvalDialog)

public:
	AEvalDialog(CWnd* pParent = NULL);   // standard constructor

   CString fNotice;

	enum { IDD = rDlgEval };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

private:
   CBitmap fBitmap;
	CStatic fCompanyLogo;
};
