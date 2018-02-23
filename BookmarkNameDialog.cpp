// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "BookmarkNameDialog.h"
#include "bookmark.h"


IMPLEMENT_DYNAMIC(CBookmarkNameDialog, CDialog)
CBookmarkNameDialog::CBookmarkNameDialog(ABookmarks& bookmarks, CWnd* pParent /*=NULL*/) :
   CDialog(CBookmarkNameDialog::IDD, pParent),
   fBookmarks(bookmarks)
{
   fName.FormatMessage(IDS_DEFAULT_BOOKMARK_NAME, fBookmarks.size() + 1);
}

CBookmarkNameDialog::~CBookmarkNameDialog()
{
}

void CBookmarkNameDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, fName);

   if (pDX->m_bSaveAndValidate)
   {
      CString message;
      if ((fName.GetLength() > ABookmark::kMaxName) || (fName.GetLength() < 1))
	   {
         message.FormatMessage(IDP_BOOKMARK_NAME, ABookmark::kMinName, ABookmark::kMaxName);
         ::AfxMessageBox(message, MB_ICONEXCLAMATION, IDP_BOOKMARK_NAME);
		   pDX->Fail();
	   }
      else if (fBookmarks.end() != std::find(fBookmarks.begin(), fBookmarks.end(), ABookmark(fName)))
      {
         message.FormatMessage(IDP_DUPLICATE_BOOKMARK, (LPCTSTR)fName);
         AfxMessageBox(message, MB_OK | MB_ICONEXCLAMATION, IDP_DUPLICATE_BOOKMARK);
		   pDX->Fail();
      }
   }
	else if (pDX->m_idLastControl != 0 && pDX->m_bEditLastControl)
	{
      HWND hWndLastControl;
      pDX->m_pDlgWnd->GetDlgItem(pDX->m_idLastControl, &hWndLastControl);
      // limit the control max-chars automatically
      ::SendMessage(hWndLastControl, EM_LIMITTEXT, ABookmark::kMaxName, 0);
	}
}

BEGIN_MESSAGE_MAP(CBookmarkNameDialog, CDialog)
END_MESSAGE_MAP()
