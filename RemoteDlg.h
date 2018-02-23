// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class ARemoteDlg : public CDialog
{
public:
	ARemoteDlg();

	//{{AFX_DATA(ARemoteDlg)
	enum {IDD = rDlgRemote};
	//}}AFX_DATA

protected:
	//{{AFX_VIRTUAL(ARemoteDlg)
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//{{AFX_MSG(ARemoteDlg)
	//}}AFX_MSG
   DECLARE_MESSAGE_MAP()

private:
   afx_msg void OnConnect();
   afx_msg void OnAdd();
   afx_msg void OnEdit();
   afx_msg void OnSettings();
   afx_msg void OnRemove();
   afx_msg void OnSlaveChange();
   afx_msg void OnLogin();
   afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

   virtual BOOL OnInitDialog();
   void EnableControls();
   bool SelIsConnected() const;

public:
   CString fUsername;
   CString fPassword;
   CListBox fSlaves;
   virtual void OnOK();
};
