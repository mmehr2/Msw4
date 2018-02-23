// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "ScriptQueueDlg.h"
#include "Utils.h"
#include "Msw.h"
#include "MswView.h"


static int gColumns[] = {0,0,rStrScriptQName,rStrScriptQErt,rStrScriptQArt};
enum {kScrollCol, kBookmarkCol, kNameCol, kErtCol, kArtCol};
enum {kUnCheckedIco=1, kCheckedIco, kBookmarkIco, kPopupIco};

IMPLEMENT_DYNAMIC(AScriptQueueDlg, CDialog)

AScriptQueueDlg::AScriptQueueDlg(CWnd* pParent /*=NULL*/)
	: CDialog(AScriptQueueDlg::IDD, pParent),
   fRefreshing(false)
{
}

AScriptQueueDlg::~AScriptQueueDlg()
{
   mDebugOnly(CheckSync());
}

void AScriptQueueDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, rCtrlList, fList);
   DDX_Control(pDX, rCtrlStatus, fStatusBar);
   DDX_Control(pDX, rCtrlStatus2, fStatusBar2);
}

void AScriptQueueDlg::RestorePosition()
{
   WINDOWPLACEMENT empty;
   ::memset(&empty, 0, sizeof(empty));
   if (0 != ::memcmp(&empty, &theApp.m_scriptQWindowPlacement, sizeof(empty)))
   {
      theApp.m_scriptQWindowPlacement.showCmd = SW_HIDE;
      this->SetWindowPlacement(&theApp.m_scriptQWindowPlacement);
   }
}

BEGIN_MESSAGE_MAP(AScriptQueueDlg, CDialog)
   ON_WM_SIZE()
   ON_WM_CLOSE()
   ON_WM_DESTROY()
   ON_WM_SHOWWINDOW()
   ON_NOTIFY(NM_DBLCLK, rCtrlList, &OnNMDoubleClick)
   ON_NOTIFY(LVN_ITEMCHANGED, rCtrlList, &OnItemChanged)
   ON_NOTIFY(NM_CLICK, rCtrlList, &OnClick)
   ON_WM_DROPFILES()
   ON_NOTIFY(LVN_COLUMNCLICK, rCtrlList, &OnColumnClick)
   ON_NOTIFY(NM_CUSTOMDRAW, rCtrlList, OnCustomDraw)
END_MESSAGE_MAP()

void AScriptQueueDlg::OnSize(UINT nType, int cx, int cy)
{
   CDialog::OnSize(nType, cx, cy);

   if (::IsWindow(fList.m_hWnd))
   {  // resize controls to fill window
      CRect rClient, rList, rStatus;
      this->GetClientRect(rClient);
      fList.GetWindowRect(rList);
      this->ScreenToClient(rList);
      fStatusBar.GetWindowRect(rStatus);
      this->ScreenToClient(rStatus);

      fStatusBar.SetWindowPos(&wndTop, rStatus.left, 
         rClient.bottom - (rStatus.Height() + rList.top), 0, 0, SWP_NOZORDER | SWP_NOSIZE);

      fStatusBar2.SetWindowPos(&wndTop, rStatus.right + 2,
         rClient.bottom - (rStatus.Height() + rList.top), 
         rClient.Width() - (rStatus.right + rList.left), rStatus.Height(), SWP_NOZORDER);

      fList.SetWindowPos(&wndTop, 0, 0, 
         rClient.Width() - 2 * rList.left, 
         rClient.Height() - ((3 * rList.top) + rStatus.Height()),
         SWP_NOMOVE | SWP_NOZORDER);

      this->UpdateStatus();
      mDebugOnly(CheckSync());
   }
}

void AScriptQueueDlg::OnNMDoubleClick(NMHDR* pNMHDR, LRESULT *pResult)
{  // toggle the check box for this item
   const int item = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR)->iItem;
   if (item >= 0)
   {
      VERIFY(fList.SetCheck(item, !fList.GetCheck(item)));
      this->WarnIfNotLinking();
   }

   *pResult = 0;
}

void AScriptQueueDlg::OnDestroy()
{
   this->GetWindowPlacement(&theApp.m_scriptQWindowPlacement);
   CDialog::OnDestroy();
}

BOOL AScriptQueueDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   fIcon = theApp.LoadIcon(rIcoMainFrame);
   this->SetIcon(fIcon, TRUE);
   this->SetIcon(fIcon, FALSE);

   // create the header
   for (int i = 0; i < mCountOf(gColumns); ++i)
   {
      CString text;
      text.LoadString(gColumns[i]);
      fList.InsertColumn(i, text);
   }

   fList.SetExtendedStyle(fList.GetExtendedStyle() | 
      LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);

   // create the image lists
	VERIFY(fListImages.Create(rBmpCheckboxes, 16, 3, RGB(255,0,255)));
   fList.SetImageList(&fListImages, LVSIL_SMALL);
	VERIFY(fHeaderImages.Create(rBmpCheckboxes, 16, 3, RGB(255,0,255)));
	fList.GetHeaderCtrl()->SetImageList(&fHeaderImages);
   fList.GetHeaderCtrl()->SetBitmapMargin(1);
	
	HDITEM hdItem;
	hdItem.mask = HDI_IMAGE | HDI_FORMAT;
	VERIFY(fList.GetHeaderCtrl()->GetItem(0, &hdItem));
	hdItem.iImage = kUnCheckedIco;
	hdItem.fmt |= HDF_IMAGE;
	VERIFY(fList.GetHeaderCtrl()->SetItem(0, &hdItem));

	VERIFY(fList.GetHeaderCtrl()->GetItem(1, &hdItem));
	hdItem.iImage = kBookmarkIco;
	hdItem.fmt |= HDF_IMAGE;
	VERIFY(fList.GetHeaderCtrl()->SetItem(1, &hdItem));

   this->UpdateStatus();
   mDebugOnly(CheckSync());

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void AScriptQueueDlg::RefreshList()
{  // we want to auto-size only after the first element is added
   fRefreshing = true;
   bool empty = (0 == fList.GetItemCount());
   fList.DeleteAllItems();
   AScriptQueue& q = theApp.fScriptQueue;
   for (size_t i = 0; i < q.size(); ++i)
   {  // store the index in the lParam
      const int row = fList.InsertItem(LVIF_PARAM, i, NULL, 0, 0, 0, i);
      VERIFY(fList.SetCheck(i, q[i].GetScroll()));
      VERIFY(fList.SetItem(row, kBookmarkCol, LVIF_IMAGE, NULL, kPopupIco, 0, 0, 0));
      VERIFY(fList.SetItemText(row, kNameCol, q[i].GetName()));

      VERIFY(fList.SetItemText(row, kErtCol, Utils::FormatDuration(q[i].GetErt())));
      VERIFY(fList.SetItemText(row, kArtCol, Utils::FormatDuration(q[i].GetArt())));
   }

   if (empty && (1 == fList.GetItemCount()))
   {  // only resize columns after the first entry
      int totalWidth = 0;
      for (int i = 0; i < mCountOf(gColumns); ++i)
      {
         VERIFY(fList.SetColumnWidth(i, LVSCW_AUTOSIZE));
         totalWidth += fList.GetColumnWidth(i);
      }

      // now resize the 'name' column to fill the extra space
      CRect client;
      fList.GetClientRect(client);
      int nameWidth = fList.GetColumnWidth(kNameCol);
      VERIFY(fList.SetColumnWidth(kNameCol, nameWidth + (client.Width() - totalWidth)));
   }

   this->UpdateStatus();
   mDebugOnly(CheckSync());
   fRefreshing = false;
}

void AScriptQueueDlg::UpdateTimes()
{
   mDebugOnly(CheckSync());
   if (!::IsWindow(fList.m_hWnd))
      return;

   AScriptQueue& q = theApp.fScriptQueue;
   for (size_t i = 0; i < q.size(); ++i)
   {
      VERIFY(fList.SetItemText(i, kErtCol, Utils::FormatDuration(q[i].GetErt())));
      VERIFY(fList.SetItemText(i, kArtCol, Utils::FormatDuration(q[i].GetArt())));
   }

   this->UpdateStatus();
}

void AScriptQueueDlg::UpdateNames()
{
   if (!::IsWindow(fList.m_hWnd))
      return;

   AScriptQueue& q = theApp.fScriptQueue;
   for (size_t i = 0; i < q.size(); ++i)
      VERIFY(fList.SetItemText(i, kNameCol, q[i].GetName()));

   mDebugOnly(CheckSync());
}

void AScriptQueueDlg::OnItemChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
   NM_LISTVIEW& nm = *(NM_LISTVIEW*)pNMHDR;
   *pResult = 0;

   if ((0 == nm.uOldState) && (0 == nm.uNewState))
      return;    // No change

   const bool wasSelected = (nm.uOldState & LVIS_SELECTED) != 0;
   const bool  isSelected = (nm.uNewState & LVIS_SELECTED) != 0;
   if (isSelected && !wasSelected && !fList.IsDragging())
   {  // a new item has been selected
      mDebugOnly(CheckSync());
      theApp.fScriptQueue.Activate(nm.iItem);
   }

   // Old check box state
   BOOL bPrevState = (BOOL)(((nm.uOldState & LVIS_STATEIMAGEMASK)>>12) - 1);
   if (bPrevState < 0)    // On startup there's no previous state 
      return;

   // New check box state
   BOOL bChecked = (BOOL)(((nm.uNewState & LVIS_STATEIMAGEMASK)>>12) - 1);
   if (bChecked < 0) // On non-checkbox notifications assume false
      return; 

   if (bPrevState == bChecked) // No change in check box
      return;

   if (nm.iItem < (int)theApp.fScriptQueue.size())
   {  // update the header indicator
		bool allChecked = true;
		for (int i = 0; allChecked && (i < fList.GetItemCount()); ++i)
         allChecked = fList.GetCheck(i) ? true : false;
		
      HDITEM hdItem;
		hdItem.mask = HDI_IMAGE;
      hdItem.iImage = (allChecked) ? kCheckedIco : kUnCheckedIco;
		VERIFY(fList.GetHeaderCtrl()->SetItem(0, &hdItem));

      // and update the script queue
      theApp.fScriptQueue[nm.iItem].SetScroll(bChecked ? true : false);
      this->UpdateStatus();

      if (bChecked && !fRefreshing)
         this->WarnIfNotLinking();
   }
}

void AScriptQueueDlg::UpdateStatus()
{
   int nScriptsToScroll = 0;
   DWORD ert = 0, art = 0;
   for (size_t i = 0; i < theApp.fScriptQueue.size(); ++i)
   {
      if (theApp.fScriptQueue[i].GetScroll())
      {
         ++nScriptsToScroll;
         ert += theApp.fScriptQueue[i].GetErt();
         art += theApp.fScriptQueue[i].GetArt();
      }
   }

   CString text;
   text.FormatMessage(rStrScriptsQueued, nScriptsToScroll);
   fStatusBar.SetWindowText(text);

   text.FormatMessage(rStrScriptsRunTime, 
      Utils::FormatDuration(ert), Utils::FormatDuration(art));
   fStatusBar2.SetWindowText(text);
}

void AScriptQueueDlg::OnDropFiles(HDROP hDropInfo)
{
   if (fList.m_hWnd == (HWND)hDropInfo)
   {  // this is a message from the list - drag/drop is complete
      // check ordering of scripts in case the user drag/dropped
      AScriptQueue temp;
      const int nItems = fList.GetItemCount();
      ASSERT(nItems > 0);
      ASSERT((size_t)nItems == theApp.fScriptQueue.size());
      for (int i = 0; i < nItems; ++i)
      {
         DWORD oldPos = fList.GetItemData(i);
         temp.Add(theApp.fScriptQueue[oldPos]);
         fList.SetItemData(i, i);
      }

      theApp.fScriptQueue = temp;
      mDebugOnly(CheckSync());
   }

   CDialog::OnDropFiles(hDropInfo);
}

void AScriptQueueDlg::OnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
   NMITEMACTIVATE* nm = (NMITEMACTIVATE*)pNMHDR;
   if ((kBookmarkCol == nm->iSubItem) && (nm->iItem >= 0))
   {  // display the bookmarks menu
      CMenu menu;
      VERIFY(menu.CreatePopupMenu());
      
      // add a title to the menu
      CString title;
      AScript script = theApp.fScriptQueue[nm->iItem];
      title.FormatMessage(rStrScriptBookmarksMenu, (LPCTSTR)script.GetName());
      menu.AppendMenu(MF_GRAYED, 0, title);
      menu.AppendMenu(MF_SEPARATOR);

      // now add the bookmarks
      script.GetView()->fRtfHelper.BuildBookmarkMenu(menu);

      // and display the menu
      POINT pt;
      ::GetCursorPos(&pt);
      menu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 
         script.GetView(), NULL);
      fList.Invalidate();  // redraw so we can hilight the new current view
   }

   *pResult = 0;
}

void AScriptQueueDlg::OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* nm = (NM_LISTVIEW*)pNMHDR;
   *pResult = 0;

   // if the 'scroll' header was clicked, toggle the bitmap and
   // select / unselect all scripts
	int i = nm->iSubItem;
   if (kScrollCol != i)
		return;

	HDITEM hdItem;
	hdItem.mask = HDI_IMAGE;
	VERIFY(fList.GetHeaderCtrl()->GetItem(i, &hdItem));

   hdItem.iImage = (hdItem.iImage == kUnCheckedIco) ? kCheckedIco : kUnCheckedIco;
	VERIFY(fList.GetHeaderCtrl()->SetItem(i, &hdItem));
	
	const bool scroll = (kCheckedIco == hdItem.iImage);
	for(i = 0; i < fList.GetItemCount(); ++i)
		fList.SetCheck(i, scroll);

   if (scroll)
      this->WarnIfNotLinking();
}

#ifdef _DEBUG
void AScriptQueueDlg::CheckSync() const
{  // make sure our list is sync'ed with the script queue
   if (!::IsWindow(fList.m_hWnd))
      return;

   const int nItems = fList.GetItemCount();
   AScriptQueue& q = theApp.fScriptQueue;
   ASSERT((size_t)nItems == q.size());
   for (int i = 0; i < nItems; ++i)
   {
      ASSERT((DWORD)i == fList.GetItemData(i));
      ASSERT(q[i].GetName() == fList.GetItemText(i, kNameCol));
   }
}
#endif

BOOL AScriptQueueDlg::PreTranslateMessage(MSG* pMsg)
{
   if ((VK_ESCAPE != pMsg->wParam) && (WM_KEYDOWN != pMsg->message))
      return CDialog::PreTranslateMessage(pMsg);
   else if ((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST))
      return ::AfxGetMainWnd()->PreTranslateMessage(pMsg);

   return FALSE;
}

void AScriptQueueDlg::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
   NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

   // Take the default processing unless we set this to something else below.
   *pResult = CDRF_DODEFAULT;

   // First thing - check the draw stage. If it's the control's prepaint
   // stage, then tell Windows we want messages for every item.
   if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
      *pResult = CDRF_NOTIFYITEMDRAW;
   else if ((CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage) && 
            (pLVCD->nmcd.dwItemSpec == theApp.fScriptQueue.GetActive()))
   {  // This is the prepaint stage for the currently active script.
      // Set the font to RED
      pLVCD->clrText = RGB(200, 0, 0);
      *pResult = CDRF_NEWFONT;
   }
}

void AScriptQueueDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
   if (!bShow)
      ::AfxGetMainWnd()->SetActiveWindow();
   CDialog::OnShowWindow(bShow, nStatus);
}

void AScriptQueueDlg::OnClose()
{  // give focus back to the application
   ::AfxGetMainWnd()->SetActiveWindow();
   CDialog::OnClose();
}

void AScriptQueueDlg::WarnIfNotLinking()
{
   if (!theApp.fLink)
   {
      size_t nSelected = 0;
      for (size_t i = 0; i < theApp.fScriptQueue.size(); ++i)
         if (theApp.fScriptQueue[i].GetScroll())
            ++nSelected;

      static bool warned = false;   // only warn once per session
      static bool autoLink = false;
      if (nSelected > 1)
      {
         if (!warned)
         {
            autoLink = (IDYES == ::AfxMessageBox(rStrWarnLink, MB_YESNO));
            warned = true;
         }

         if (autoLink)
            theApp.fLink = true;
      }
   }
}
