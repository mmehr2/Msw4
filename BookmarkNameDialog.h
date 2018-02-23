// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "resource.h"
#include "Bookmark.h"


class CBookmarkNameDialog : public CDialog
{
public:
	CBookmarkNameDialog(ABookmarks& bookmarks, CWnd* pParent = NULL);   // standard constructor
	virtual ~CBookmarkNameDialog();

	enum { IDD = rDlgBookmarkName };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(CBookmarkNameDialog)

public:
	CString fName;

private:
   ABookmarks& fBookmarks;
};
