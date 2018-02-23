// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class CStyleListDialog : public CDialog
{
private:
   DECLARE_DYNAMIC(CStyleListDialog)

public:
	CStyleListDialog(CWnd* pParent = NULL);   // standard constructor

	enum { IDD = rListStyle };
	CListBox m_lbStyles;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void UpdateList();
	void SelectByName(LPCTSTR szName);

private:
	afx_msg void OnAddStyle();
	afx_msg void OnEditStyle();
	afx_msg void OnDeleteStyle();
	virtual BOOL OnInitDialog();
   void EnableButtons();

	DECLARE_MESSAGE_MAP()
   afx_msg void OnSelectionChange() {this->EnableButtons();}
};
