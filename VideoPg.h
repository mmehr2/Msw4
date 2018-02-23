// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"


class AVideoPage : public CPropertyPage
{
public:
	AVideoPage();   // standard constructor

	//{{AFX_DATA(AVideoPage)
	enum {IDD = rDlgVideoConfig};
	//}}AFX_DATA

protected:
	//{{AFX_VIRTUAL(AVideoPage)
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//{{AFX_MSG(AVideoPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   BOOL fColor;
   BOOL fInverseVideo;
   BOOL fScriptDividers;
   BOOL fShowSpeed;
   BOOL fShowScriptPos;
   BOOL fShowTimer;
   BOOL fLoop;
   BOOL fLink;
   BOOL fRightToLeft;

private:
   afx_msg void OnDestroy();
   afx_msg void OnMirrorChanged();
   virtual BOOL OnInitDialog();

   CStatic fMirror;
   HICON fR2L, fL2R;
};
