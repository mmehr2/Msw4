// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class AGotoLineDlg : public CDialog
{
   DECLARE_DYNAMIC(AGotoLineDlg)

public:
   AGotoLineDlg(CWnd* pParent = NULL);   // standard constructor

   enum { IDD = rDlgGotoLine };

   int fLine;

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
};
