// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


class AAddSlaveDlg : public CDialog
{
public:
	AAddSlaveDlg(CWnd* pParent = NULL);   // standard constructor

	enum { IDD = rDlgAddSlave };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

	DECLARE_DYNAMIC(AAddSlaveDlg)
public:
   CString fInstallation;
   CString fUsername;
};
