// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


#include "Msw.h"

class AChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(AChildFrame)
public:
   AChildFrame() : CMDIChildWnd() {}

	//{{AFX_VIRTUAL(AChildFrame)
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(AChildFrame)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
