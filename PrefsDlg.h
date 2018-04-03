// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "GeneralPrefsDlg.h"
#include "ScrollPrefsDlg.h"
#include "VideoPg.h"


class APrefsDlg : public CPropertySheet
{
private:
   DECLARE_DYNAMIC(APrefsDlg)

public:
	APrefsDlg(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	APrefsDlg(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

   virtual BOOL OnInitDialog();

   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

protected:
	DECLARE_MESSAGE_MAP()
   virtual void OnHelp();

public:  // data
   AGeneralPrefsDlg fGeneralPage;
   AScrollPrefsDlg fScrollPage;
   AVideoPage fVideoPage;

   CStringA Serialize() const;
   void Deserialize( const CStringA& cs );
   bool UnitTestSerial() const;
};


