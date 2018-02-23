// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "childfrm.h"

IMPLEMENT_DYNCREATE(AChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(AChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(AChildFrame)
   ON_COMMAND(rCmdFileClose, CMDIChildWnd::OnClose)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

