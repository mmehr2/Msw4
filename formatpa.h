// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"

class AFormatParaDlg : public CDialog
{
public:
	AFormatParaDlg(const CParaFormat& pf, CWnd* pParent = NULL);   // standard constructor
	CParaFormat m_pf;

	//{{AFX_DATA(AFormatParaDlg)
	enum { IDD = rDlgParaFormat};
	//}}AFX_DATA

protected:
   WORD GetAlignment(int index) const;
   int GetAlignmentIndex(int align) const;
   BYTE GetSpacing(int index) const;
   int GetSpacingIndex(int spacing) const;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	//{{AFX_MSG(AFormatParaDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int m_nAlignment;
	int m_nSpacing;
};
