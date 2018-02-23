// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class AFindReplaceDlg : public CFindReplaceDialog
{
private:
   DECLARE_DYNAMIC(AFindReplaceDlg)

public:
	AFindReplaceDlg(CWnd* pParent = NULL);   // standard constructor

	enum { IDD = rDlgFindReplace };

   afx_msg void OnUseFindFormat();
   afx_msg void OnUseReplaceFormat();
   afx_msg void OnFindFormat() {ASSERT(false);}
   afx_msg void OnReplaceFormat();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   int fUseFindFormat;
   int fUseReplaceFormat;
   CCharFormat fFindFormat;
   CCharFormat fReplaceFormat;
};
