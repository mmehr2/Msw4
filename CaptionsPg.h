// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class ACaptionsPage : public CPropertyPage
{
public:
	ACaptionsPage();   // standard constructor

	//{{AFX_DATA(ACaptionsPage)
	enum {IDD = rDlgCaptionsConfig};
	//}}AFX_DATA

protected:
	//{{AFX_VIRTUAL(ACaptionsPage)
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//{{AFX_MSG(ACaptionsPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   int fEnabled;
   CString fPort;
   int fParity;
   int fStopBits;
   int fBaudRate;
   int fByteSize;

private:
   virtual BOOL OnInitDialog();
   CComboBox fPortList;
   afx_msg void OnEnable();
   void Enable();
};
