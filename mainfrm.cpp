// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"

#include <dbt.h>

#include "colorlis.h"
#include "mainfrm.h"
#include "mswdoc.h"
#include "mswview.h"
#include "PrefsDlg.h"
#include "strings.h"
#include ".\mainfrm.h"
#include "scrollDialog.h"
#include "RemoteDlg.h"
#include "RtfHelper.h"
#include "Utils.h"

#ifdef REGISTER
#  include "RegisterDlg.h"
#endif // REGISTER

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(AMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(AMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(AMainFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_COMMAND(ID_HELP, OnHelpFinder)
	ON_WM_DROPFILES()
	ON_COMMAND(ID_CHAR_COLOR, OnCharColor)
	ON_WM_FONTCHANGE()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
	ON_WM_DEVMODECHANGE()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_DESTROY()
//	ON_COMMAND(rCmdListen, OnListen)
//	ON_COMMAND(rCmdVoiceTraining, OnVoiceTraining)
//	ON_COMMAND(rCmdMicSetup, OnMicSetup)
//	ON_UPDATE_COMMAND_UI(rCmdListen, OnUpdateListen)
//	ON_UPDATE_COMMAND_UI(rCmdVoiceTraining, OnUpdateVoiceTraining)
//	ON_UPDATE_COMMAND_UI(rCmdMicSetup, OnUpdateMicSetup)
// ON_MESSAGE(ASRManager::rCmdVoice, OnVoiceCommand)
	ON_COMMAND(ID_HELP_INDEX, OnHelpFinder)
	//}}AFX_MSG_MAP
	// Global help commands
//  ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
	ON_UPDATE_COMMAND_UI(rCmdViewFormatbar, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(rCmdViewIconBar, OnUpdateControlBarMenu)
	ON_REGISTERED_MESSAGE(AMswApp::sOpenMsg, OnOpenMsg)
	ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnBarCheck)
	ON_COMMAND_EX(rCmdViewToolbar, OnBarCheck)
	ON_COMMAND_EX(rCmdViewIconBar, OnBarCheck)
	ON_COMMAND_EX(rCmdViewFormatbar, OnBarCheck)
   ON_COMMAND(rCmdToggleScrollMode, OnScroll)
   ON_UPDATE_COMMAND_UI(rCmdToggleScrollMode, &OnUpdateToggleScrollMode)
   ON_COMMAND(rCmdToggleInverseOut, OnInverseOut)
   ON_UPDATE_COMMAND_UI(rCmdToggleInverseOut, &OnUpdateToggleInverseOut)
   ON_COMMAND(rCmdToggleInverseEdit, OnInverseEdit)
   ON_UPDATE_COMMAND_UI(rCmdToggleInverseEdit, &OnUpdateToggleInverseEdit)
   ON_WM_SHOWWINDOW()
   ON_WM_CLOSE()
   ON_MESSAGE(WM_MDIMAXIMIZE, OnMDIMaximize)
	ON_MESSAGE(WMA_UPDATE_STYLES, UpdateStyles)
   ON_COMMAND(rCmdHelpWeb, &OnWebLink)
   ON_COMMAND(rCmdViewShowLineNum, &OnShowLineNumbers)
   ON_COMMAND(rCmdToggleTimer, &OnTimerOn)
   ON_UPDATE_COMMAND_UI(rCmdViewShowLineNum, &OnUpdateShowLineNumbers)
   ON_UPDATE_COMMAND_UI(rCmdToggleTimer, &OnUpdateTimerOn)
   ON_COMMAND(rCmdEditPrefs, &OnPreferences)
#ifdef _REMOTE
   ON_COMMAND(rCmdRemote, &OnRemote)
#endif // _REMOTE
   ON_COMMAND(rCmdViewQueue, &OnViewScriptQueue)
   ON_UPDATE_COMMAND_UI(rCmdViewQueue, &OnUpdateViewScriptQueue)
   ON_WM_DEVICECHANGE()
	ON_WM_TIMER()
   ON_COMMAND(rCmdTimerReset, &OnResetTimer)
   ON_COMMAND(rCmdLink, &OnLink)
   ON_COMMAND(rCmdLoop, &OnLoop)
   ON_UPDATE_COMMAND_UI(rCmdLink, &OnUpdateLink)
   ON_UPDATE_COMMAND_UI(rCmdLoop, &OnUpdateLoop)
#ifdef REGISTER
   ON_COMMAND(rCmdHelpRegisterMsw, &OnRegister)
#endif // REGISTER
   ON_COMMAND(rCmdViewRuler, &OnViewRuler)
   ON_UPDATE_COMMAND_UI(rCmdViewRuler, &AMainFrame::OnUpdateViewRuler)
   ON_WM_MEASUREITEM()
   ON_WM_MENUCHAR()
   ON_WM_INITMENUPOPUP()
   ON_UPDATE_COMMAND_UI(ID_CHAR_COLOR, &OnUpdateFormatCommand)
   ON_UPDATE_COMMAND_UI(IDC_FONTNAME, &OnUpdateFormatCommand)
   ON_UPDATE_COMMAND_UI(IDC_FONTSIZE, &OnUpdateFormatCommand)
   ON_UPDATE_COMMAND_UI(IDC_FONTSTYLE, &OnUpdateFormatCommand)
END_MESSAGE_MAP()

// toolbar buttons - IDs are command buttons
static UINT BASED_CODE sToolbar[] =
{  // same order as in the bitmap 'toolbar.bmp'
	rCmdFileNew, rCmdFileOpen, rCmdFileClose, rCmdFileSave,
ID_SEPARATOR,
	rCmdPrintDirect, rCmdFilePrintPreview,
ID_SEPARATOR,
	rCmdEditCut, rCmdEditCopy, rCmdEditPaste, rCmdEditUndo, 
ID_SEPARATOR,
	rCmdEditFind,
ID_SEPARATOR,
   rCmdHelpTopics,
};

static UINT BASED_CODE sIconbar[] =
{  // same order as in the bitmap 'toolbar.bmp'
   rCmdFormatFontBtn,
   rCmdEditFind,
	rCmdBookmarks,
   rCmdToggleScrollMode,
   rCmdToggleInverseOut,
   rCmdToggleInverseEdit,
ID_SEPARATOR,
   rCmdTimerToArt,
ID_SEPARATOR,
   rCmdLoop,
//   rCmdOut,
   rCmdLink
};

static UINT BASED_CODE sFormat[] =
{  // same order as in the bitmap 'format.bmp'
   ID_SEPARATOR, // style combo box
ID_SEPARATOR,
   ID_SEPARATOR, // font name combo box
ID_SEPARATOR,
   ID_SEPARATOR, // font size combo box
ID_SEPARATOR,
   rCmdSizeLarger,
   rCmdSizeSmaller,
ID_SEPARATOR,
   ID_CHAR_BOLD, ID_CHAR_ITALIC, ID_CHAR_UNDERLINE,
ID_SEPARATOR,
// black       white        red         green        yellow       blue        cycan
   rCmdColor0, rCmdColor15, rCmdColor9, rCmdColor10, rCmdColor11, rCmdColor4, rCmdColor14, ID_CHAR_COLOR, 
ID_SEPARATOR,
   ID_PARA_LEFT, ID_PARA_CENTER, ID_PARA_RIGHT,
ID_SEPARATOR,
   rCmdFormatBullets
};

static UINT BASED_CODE indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
};

BOOL AMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	WNDCLASS wndcls;

	BOOL bRes = CMDIFrameWnd::PreCreateWindow(cs);
	HINSTANCE hInst = AfxGetInstanceHandle();

	// see if the class already exists
	if (!::GetClassInfo(hInst, gStrMswClass, &wndcls))
	{
		// get default stuff
		::GetClassInfo(hInst, cs.lpszClass, &wndcls);
		wndcls.style &= ~(CS_HREDRAW|CS_VREDRAW);
		// register a new class
		wndcls.lpszClassName = gStrMswClass;
		wndcls.hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(rMainFrame));
		ASSERT(wndcls.hIcon != NULL);
		if (!AfxRegisterClass(&wndcls))
			AfxThrowResourceException();
	}
	cs.lpszClass = gStrMswClass;
	return bRes;
}

int AMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (!this->CreateToolBar())
		return -1;
	if (!this->CreateIconBar())
		return -1;
	if (!this->CreateFormatBar())
		return -1;
	if (!this->CreateStatusBar())
		return -1;

   ::AfxEnableControlContainer();

   EnableDocking(CBRS_ALIGN_ANY);
   m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
   m_wndIconBar.EnableDocking(CBRS_ALIGN_ANY);
   m_wndFormatBar.EnableDocking(CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);

   DockControlBar(&m_wndToolBar);

   // now dock the icon bar to the right of the tool bar
   // get MFC to adjust the dimensions of all docked ToolBars so that GetWindowRect will be accurate
   RecalcLayout(TRUE);
   CRect rect;
   m_wndToolBar.GetWindowRect(&rect);
   rect.OffsetRect(1,0);

   // When we take the default parameters on rect, DockControlBar
   // will dock each Toolbar on a separate line. By calculating a
   // rectangle, we are simulating a Toolbar being dragged to that
   // location and docked.
   DockControlBar(&m_wndIconBar, m_wndToolBar.GetBarStyle(), &rect);
   DockControlBar(&m_wndFormatBar);

   this->LoadBarState(gStrBarState);

	CWnd* pView = GetDlgItem(AFX_IDW_PANE_FIRST);
	if (pView != NULL)
		pView->SetWindowPos(&wndBottom, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);

   // create the script queue
   theApp.fScriptQueueDlg.Create(AScriptQueueDlg::IDD, this);
   theApp.fScriptQueueDlg.RestorePosition();

/*
   // voice activation
   if (fSpeech.Initialize(this->m_hWnd))
      fSpeech.SetIsActive(theApp.m_bVoiceActivated);
   else
      TRACE(("Error creating speech recognition engine\n"));
*/

   this->SetTimer(1, 250, NULL);

   return 0;
}

BOOL AMainFrame::CreateToolBar()
{
	if (!m_wndToolBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_ALIGN_TOP |
      CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, rCmdViewToolbar) || 
       !m_wndToolBar.LoadToolBar(rTbMain) ||
       !m_wndToolBar.SetButtons(sToolbar, mCountOf(sToolbar)))
		return FALSE;      // fail to create

   CString text;
   VERIFY(text.LoadString(rStrToolBarCaption));
   m_wndToolBar.SetWindowText(text);
   VERIFY(m_wndToolBar.ModifyStyle(0, TBSTYLE_FLAT));
   return TRUE;
}

BOOL AMainFrame::CreateIconBar()
{
	if (!m_wndIconBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_ALIGN_TOP |
      CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, rCmdViewIconBar) || 
       !m_wndIconBar.LoadToolBar(rTbIconBig) ||
       !m_wndIconBar.SetButtons(sIconbar, mCountOf(sIconbar)))
		return FALSE;      // fail to create
   
   CString text;
   VERIFY(text.LoadString(rStrControlPanelCaption));
   m_wndIconBar.SetWindowText(text);
   VERIFY(m_wndIconBar.ModifyStyle(0, TBSTYLE_FLAT));

   //set up the timer control as a static text control
   //First get the index of the placeholder's position in the toolbar
   int iTimer = 0;
   while (rCmdTimerToArt != m_wndIconBar.GetItemID(iTimer))
      ++iTimer;

   // next convert the button to a seperator and get its position
   CRect r;
   UINT id, style;
   int image;
   m_wndIconBar.GetButtonInfo(0, id, style, image);
   m_wndIconBar.SetButtonInfo(iTimer, rCmdTimerToArt, TBBS_SEPARATOR, 50);
   m_wndIconBar.GetItemRect(iTimer, &r);
   if (!m_wndIconBar.fTimer.Create(_T(""), BS_PUSHBUTTON|BS_MULTILINE|WS_CHILD|WS_VISIBLE, 
      r, &m_wndIconBar, rCmdTimerToArt))
      return FALSE;
   m_wndIconBar.fTimer.SendMessage(WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT));
   this->SetArtTimer(fTimer);

   return TRUE;
}

BOOL AMainFrame::CreateFormatBar()
{
	if (!m_wndFormatBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER |
      CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, rCmdViewFormatbar) || 
       !m_wndFormatBar.LoadToolBar(rTbFormat) ||
       !m_wndFormatBar.SetButtons(sFormat, mCountOf(sFormat)))
		return FALSE;      // fail to create

	m_wndFormatBar.PositionCombos();

   CString text;
   VERIFY(text.LoadString(rStrFormatBarCaption));
   m_wndFormatBar.SetWindowText(text);
   VERIFY(m_wndFormatBar.ModifyStyle(0, TBSTYLE_FLAT));
   return TRUE;
}

BOOL AMainFrame::CreateStatusBar()
{
	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators,
    sizeof(indicators)/sizeof(UINT)))
		return FALSE;      // fail to create

   return TRUE;
}

void AMainFrame::OnFontChange()
{
	m_wndFormatBar.SendMessage(AMswApp::sPrinterChangedMsg);
}

void AMainFrame::OnDevModeChange(LPTSTR lpDeviceName)
{
	theApp.NotifyPrinterChanged();
	CMDIFrameWnd::OnDevModeChange(lpDeviceName); //sends message to descendants
}

void AMainFrame::OnSysColorChange()
{
	CMDIFrameWnd::OnSysColorChange();
   this->SendMessageToDescendants(WM_SYSCOLORCHANGE);
}

void AMainFrame::ActivateFrame(int nCmdShow)
{
	CMDIFrameWnd::ActivateFrame(nCmdShow);
	// make sure and display the toolbar, ruler, etc while loading a document.
	OnIdleUpdateCmdUI();
	UpdateWindow();
}

void AMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIFrameWnd::OnSize(nType, cx, cy);
	theApp.m_bMaximized = (nType == SIZE_MAXIMIZED);

   if (nType != SIZE_MINIMIZED)
   {  // make sure the scroll margins are inside the screen
      CRect screen;
      ::SystemParametersInfo(SPI_GETWORKAREA, 0, &screen, 0);
      cx = screen.Width() - (ARtfHelper::kCueBarWidth + 
         ARtfHelper::kRtfHOffset + GetSystemMetrics(SM_CXVSCROLL));
      const int minScrollMargin = max(0, min(theApp.fScrollMarginLeft, cx));
      const int maxScrollMargin = max(0, min(theApp.fScrollMarginRight, cx));
      theApp.SetScrollMargins(minScrollMargin, maxScrollMargin);
   }
}

LONG AMainFrame::OnOpenMsg(UINT, LONG lParam)
{
	TCHAR szAtomName[256];
	szAtomName[0] = NULL;
	GlobalGetAtomName((ATOM)lParam, szAtomName, 256);
	AMswDoc* pDoc = (AMswDoc*)GetActiveDocument();
	if (szAtomName[0] != NULL && pDoc != NULL)
	{
		if (lstrcmpi(szAtomName, pDoc->GetPathName()) == 0)
			return TRUE;
	}
	return FALSE;
}

void AMainFrame::OnHelpFinder()
{
   theApp.ShowHelp(HH_HELP_FINDER);
}

void AMainFrame::OnDropFiles(HDROP hDropInfo)
{  // Get the # of files being dropped.
   const UINT n = ::DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
   for (UINT i = 0; i < n; ++i)
   {  // Get the next filename from the HDROP info.
      TCHAR szNextFile[MAX_PATH];
      if (::DragQueryFile(hDropInfo, i, szNextFile, _countof(szNextFile)) > 0)
      	theApp.OpenDocumentFile(szNextFile);
   }
   ::DragFinish(hDropInfo);
}

void AMainFrame::OnCharColor()
{
	AColorMenu colorMenu;
	CRect rc;
	int index = m_wndFormatBar.CommandToIndex(ID_CHAR_COLOR);
	m_wndFormatBar.GetItemRect(index, &rc);
	m_wndFormatBar.ClientToScreen(rc);
	colorMenu.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON,rc.left,rc.bottom, this);
}

void AMainFrame::OnDestroy() 
{
   this->GetWindowPlacement(&theApp.m_windowPlacement);
   CMDIFrameWnd::OnDestroy();
}

/*
void AMainFrame::OnUpdateListen(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(fSpeech.IsAvailable());
   pCmdUI->SetCheck(fSpeech.GetIsActive());
}

void AMainFrame::OnListen() 
{
   theApp.m_bVoiceActivated = !theApp.m_bVoiceActivated;
   fSpeech.SetIsActive(theApp.m_bVoiceActivated);
}

void AMainFrame::OnUpdateVoiceTraining(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(fSpeech.SupportsUserTraining());
}

void AMainFrame::OnVoiceTraining() 
{
   fSpeech.TrainUser(this->m_hWnd);
}

void AMainFrame::OnUpdateMicSetup(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(fSpeech.SupportsMicTraining());
}

void AMainFrame::OnMicSetup() 
{
   fSpeech.TrainMic(this->m_hWnd);
}

LRESULT AMainFrame::OnVoiceCommand(WPARAM, LPARAM)
{
   CSpEvent event;
   while (fSpeech.GetEvent(event))
   {  // Loop processing events while there are any in the queue
      // Get the phrase elements, one of which is the rule id we specified in
      // the grammar.  Switch on it to figure out which command was recognized.
      SPPHRASE* elements;
      if (FAILED(event.RecoResult()->GetPhrase(&elements)))
         continue;

      const int commandType = elements->Rule.ulId;
      const int command = elements->pProperties->vValue.ulVal;
      ::CoTaskMemFree(elements);

      TRACE("Voice command (%d,%d)\n", commandType, command);
      switch (commandType)
      {
         case ASRManager::Command:
         switch (command)
         {
            case ASRManager::rCmdScroll:
            case ASRManager::rCmdPause:
            case ASRManager::rCmdStop:
            case ASRManager::rCmdEdit:
            case ASRManager::rCmdFast:
            case ASRManager::rCmdFaster:
            case ASRManager::rCmdSlow:
            case ASRManager::rCmdSlower:
            break;

            default:
            ASSERT(0);  // no other commands yet
            break;
         }
         break;

         default:
         ASSERT(0);  // no other command types yet
         break;
      }
   }
   return 0;
}
*/

void AMainFrame::OnScroll()
{
#ifdef _REMOTE
   const MSG* msg = this->GetCurrentMessage();
   if (AComm::kOff == HIWORD(msg->wParam)) { // leave scroll mode - we're already there!
      return;
   }
   if (gComm.IsMaster()) {
      gComm.SendCommand(AComm::kCueMarker, ARtfHelper::sCueMarker.GetPosition().y);
      gComm.SendCommand(AComm::kScrollMargins, theApp.fScrollMarginLeft, theApp.fScrollMarginRight);
   }
#endif // _REMOTE

   if (!theApp.IsLegalCopy())
      return;

   // turn off anti-aliasing, if it's on
   BOOL antiAliasing = FALSE;
   if (::SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &antiAliasing, 0) && antiAliasing)
      VERIFY(::SystemParametersInfo(SPI_SETFONTSMOOTHING, FALSE, NULL, 0));

   // save the current cursor position, so we can restore it afterwards
   CPoint savedCursor;
   VERIFY(::GetCursorPos(&savedCursor));

   AScrollDialog scrollDlg;
   if (!theApp.fResetTimer)
      scrollDlg.fRtfHelper.fArt = fTimer;
   theApp.fTimerOn = !theApp.fManualTimer;
   scrollDlg.DoModal();

   // make this window the target for remote commands
#ifdef _REMOTE
   gComm.SetParent(m_hWnd);
#endif // _REMOTE

   // if anti-aliasing was on before we scrolled, turn it back on
   if (antiAliasing)
      VERIFY(::SystemParametersInfo(SPI_SETFONTSMOOTHING, TRUE, NULL, 0));

   this->SetArtTimer(scrollDlg.fRtfHelper.fArt.Elapsed());
   VERIFY(::SetCursorPos(savedCursor.x, savedCursor.y));
   AGuiMarker& cue = scrollDlg.fRtfHelper.sCueMarker;
   CPoint cuePos = cue.GetPosition();
   cuePos.x = ARtfHelper::kCueBarWidth - (cue.GetSize().cx + 2);
   cue.SetPosition(cuePos);

/* // for a persistent, non-modal window...
   if (NULL == fScrollDlg.GetSafeHwnd())
   {  // create the window and maximize
      fScrollDlg.Create(CscrollDlg::IDD, this);
      fScrollDlg.ShowWindow(SW_MAXIMIZE);
      fScrollDlg.RestorePosition();
   }

   fScrollDlg.ShowWindow(SW_SHOW);
*/
}

void AMainFrame::OnUpdateToggleScrollMode(CCmdUI *pCmdUI)
{
   AScriptQueue scripts = theApp.fScriptQueue.GetQueuedScripts();
   pCmdUI->Enable(scripts.size() > 0);
}

void AMainFrame::OnInverseOut()
{
   theApp.fInverseOut = !theApp.fInverseOut;
}

void AMainFrame::OnUpdateToggleInverseOut(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(theApp.fInverseOut);
}

void AMainFrame::OnInverseEdit()
{
   theApp.fInverseEditMode = !theApp.fInverseEditMode;
   this->SendMessageToDescendants(theApp.sInverseEditModeMsg);
}

void AMainFrame::OnUpdateToggleInverseEdit(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(theApp.fInverseEditMode);
}

void AMainFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
   CMDIFrameWnd::OnShowWindow(bShow, nStatus);

   static bool doneOnce = false;
   if (bShow && !IsWindowVisible() && !doneOnce)
   {
      doneOnce = true;
      WINDOWPLACEMENT empty;
      ::memset(&empty, 0, sizeof(empty));
      if (0 != ::memcmp(&empty, &theApp.m_windowPlacement, sizeof(empty)))
         this->SetWindowPlacement(&theApp.m_windowPlacement);
   }
}

void AMainFrame::OnClose()
{
   // save the currently open files
   CString key;
   for (size_t i = 0; ; ++i)
   {
      key.Format(_T("%s%d"), (LPCTSTR)gStrLastFile, i);
      CString queuedKey = key + _T('Q');
      if (i < theApp.fScriptQueue.size())
      {
         theApp.WriteProfileString(gStrSection, key, theApp.fScriptQueue[i].GetPath());
         theApp.WriteProfileInt(gStrSection, queuedKey, theApp.fScriptQueue[i].GetScroll());
      }
      else if (!theApp.GetProfileString(gStrSection, key).IsEmpty())
         theApp.WriteProfileString(gStrSection, key, NULL);
      else
         break;
   }

   theApp.fViewScriptQueue = ::IsWindow(theApp.fScriptQueueDlg.m_hWnd) &&
      theApp.fScriptQueueDlg.IsWindowVisible();

   this->SaveBarState(gStrBarState);

   CMDIFrameWnd::OnClose();
}

LRESULT AMainFrame::OnMDIMaximize(WPARAM, LPARAM)
{  // maximize the active window
   BOOL alreadyMaximized;
   CMDIChildWnd* cw = this->MDIGetActive(&alreadyMaximized);
   if ((NULL != cw) && !alreadyMaximized) 
      this->MDIMaximize(cw);

   return 0;
}

LRESULT AMainFrame::UpdateStyles(WPARAM, LPARAM)
{
	CString style;
   int nCurSel = m_wndFormatBar.m_comboStyles.GetCurSel();
	if (nCurSel >= 0)
		m_wndFormatBar.m_comboStyles.GetLBText(nCurSel, style);

	m_wndFormatBar.FillStyleList();

   m_wndFormatBar.m_comboStyles.SetCurSel(
      m_wndFormatBar.m_comboStyles.FindStringExact(-1, style));

	return 0;
}

void AMainFrame::OnWebLink()
{
   ::ShellExecute(NULL, _T("open"), gStrMswUrl, NULL, NULL, SW_SHOWNORMAL);
}

void AMainFrame::OnShowLineNumbers()
{
   theApp.fLineNumbers = !theApp.fLineNumbers;
   this->SendMessageToDescendants(AMswApp::sRedrawCueBarMsg);
}

void AMainFrame::OnUpdateShowLineNumbers(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(theApp.fLineNumbers);
}

void AMainFrame::OnTimerOn()
{
   theApp.fTimerOn = !theApp.fTimerOn;
}

void AMainFrame::OnUpdateTimerOn(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(theApp.fTimerOn);
}

void AMainFrame::OnPreferences()
{
   APrefsDlg dlg(rStrPreferencesDlg, this);

   // load up the current prefs
   dlg.fGeneralPage.fClearPaperclips = theApp.fClearPaperclips;
   dlg.fGeneralPage.fResetTimer = theApp.fResetTimer;
   dlg.fGeneralPage.fManualTimer = theApp.fManualTimer;
   dlg.fGeneralPage.fCharsPerSecond = theApp.fCharsPerSecond / 10.0;
   dlg.fGeneralPage.fInverseEditMode = theApp.fInverseEditMode;
   dlg.fGeneralPage.fFontLanguage = theApp.fFontLanguage;

   dlg.fScrollPage.fLButton = theApp.fLButtonAction;
   dlg.fScrollPage.fRButton = theApp.fRButtonAction;
   dlg.fScrollPage.fReverseSpeedControl = theApp.fReverseSpeedControl;

   dlg.fVideoPage.fColor = theApp.fColorOut;
   dlg.fVideoPage.fInverseVideo = theApp.fInverseOut;
   dlg.fVideoPage.fScriptDividers = theApp.fShowDividers;
   dlg.fVideoPage.fShowSpeed = theApp.fShowSpeed;
   dlg.fVideoPage.fShowScriptPos = theApp.fShowPos;
   dlg.fVideoPage.fShowTimer = theApp.fShowTimer;
   dlg.fVideoPage.fLoop = theApp.fLoop;
   dlg.fVideoPage.fLink = theApp.fLink;
   dlg.fVideoPage.fRightToLeft = theApp.fMirror;

   /*
   dlg.fCaptionsPage.fPort = theApp.fCaption.fPortNum;
   dlg.fCaptionsPage.fEnabled = theApp.fCaption.fEnabled;
   dlg.fCaptionsPage.fBaudRate = theApp.fCaption.fBaudRate;
   dlg.fCaptionsPage.fByteSize = theApp.fCaption.fByteSize;
   dlg.fCaptionsPage.fParity = theApp.fCaption.fParity;
   dlg.fCaptionsPage.fStopBits = theApp.fCaption.fStopBits;
   */

   if (IDOK == dlg.DoModal())
   {  // save 'em
      theApp.fClearPaperclips = dlg.fGeneralPage.fClearPaperclips ? true : false;
      theApp.fResetTimer = dlg.fGeneralPage.fResetTimer ? true : false;
      theApp.fManualTimer = dlg.fGeneralPage.fManualTimer ? true : false;
      const int charsPerSec = static_cast<int>(dlg.fGeneralPage.fCharsPerSecond * 10.0);
      if (charsPerSec != theApp.fCharsPerSecond)
      {  // redraw the script dialog
         theApp.fCharsPerSecond = charsPerSec;
         theApp.fScriptQueueDlg.UpdateTimes();
      }
      const bool inverseEdit = dlg.fGeneralPage.fInverseEditMode ? true : false;
      if (theApp.fInverseEditMode != inverseEdit)
      {  // redraw all views
         theApp.fInverseEditMode = inverseEdit;
         this->SendMessageToDescendants(theApp.sInverseEditModeMsg);
      }
      theApp.fFontLanguage = dlg.fGeneralPage.fFontLanguage;
      m_wndFormatBar.FillFontList();

      theApp.fLButtonAction = dlg.fScrollPage.fLButton;
      theApp.fRButtonAction = dlg.fScrollPage.fRButton;
      theApp.fReverseSpeedControl = dlg.fScrollPage.fReverseSpeedControl ? true : false;

      theApp.fColorOut = dlg.fVideoPage.fColor ? true : false;
      theApp.fInverseOut = dlg.fVideoPage.fInverseVideo ? true : false;
      theApp.fShowDividers = dlg.fVideoPage.fScriptDividers ? true : false;
      theApp.fShowSpeed = dlg.fVideoPage.fShowSpeed ? true : false;
      theApp.fShowPos = dlg.fVideoPage.fShowScriptPos ? true : false;
      theApp.fShowTimer = dlg.fVideoPage.fShowTimer ? true : false;
      theApp.fLoop = dlg.fVideoPage.fLoop ? true : false;
      theApp.fLink = dlg.fVideoPage.fLink ? true : false;
      theApp.fMirror = dlg.fVideoPage.fRightToLeft ? true : false;

      /*
      theApp.fCaption.fPortNum = dlg.fCaptionsPage.fPort;
      theApp.fCaption.fEnabled = dlg.fCaptionsPage.fEnabled;
      theApp.fCaption.fBaudRate = dlg.fCaptionsPage.fBaudRate;
      theApp.fCaption.fByteSize = dlg.fCaptionsPage.fByteSize;
      theApp.fCaption.fParity = dlg.fCaptionsPage.fParity;
      theApp.fCaption.fStopBits = dlg.fCaptionsPage.fStopBits;
      */
   }
}

#ifdef _REMOTE
void AMainFrame::OnRemote()
{
   ARemoteDlg().DoModal();
}
#endif // _REMOTE

void AMainFrame::OnViewScriptQueue()
{
   theApp.fViewScriptQueue = !theApp.fScriptQueueDlg.IsWindowVisible();
   theApp.fScriptQueueDlg.ShowWindow(theApp.fViewScriptQueue ? SW_SHOW : SW_HIDE);
}

void AMainFrame::OnUpdateViewScriptQueue(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(::IsWindow(theApp.fScriptQueueDlg.m_hWnd) && 
      theApp.fScriptQueueDlg.IsWindowVisible());
}

void AMainFrame::OnLink()
{
   theApp.fLink = !theApp.fLink;
}

void AMainFrame::OnLoop()
{
   theApp.fLoop = !theApp.fLoop;
}

void AMainFrame::OnUpdateLink(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(theApp.fLink);
}

void AMainFrame::OnUpdateLoop(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(theApp.fLoop);
}

BOOL AMainFrame::OnDeviceChange(UINT nEventType, DWORD_PTR)
{
   if (DBT_DEVNODES_CHANGED == nEventType)
      theApp.fHandController.Connect();

   return TRUE;
}

void AMainFrame::OnTimer(UINT)
{  // wait a bit if we just left scroll mode. If we check the hand 
   // controller too soon, we could get a false scroll event.
   if ((AMswApp::kScroll == theApp.fLButtonAction) || 
       (AMswApp::kScroll == theApp.fRButtonAction))
   {
      int nScroll = 0;
      bool lClick = false, rClick = false;
      if (theApp.fHandController.Poll(nScroll, lClick, rClick))
      {
         if (lClick && (AMswApp::kScroll == theApp.fLButtonAction))
            this->OnScroll();
         else if (rClick && (AMswApp::kScroll == theApp.fRButtonAction))
            this->OnScroll();
      }
   }
}

void AMainFrame::OnResetTimer()
{
   this->SetArtTimer(0);
}

void AMainFrame::OnRegister()
{
#ifdef REGISTER
   ARegisterDlg dlg(theApp.fReg.GetName(), theApp.fReg.GetNumber());
   if (IDOK == dlg.DoModal())
      theApp.SetRegistration(dlg.fReg.GetName(), dlg.fReg.GetNumber());
#endif // REGISTER
}

void AMainFrame::OnViewRuler()
{
   theApp.fShowRuler = !theApp.fShowRuler;
   this->SendMessageToDescendants(theApp.sShowRulerMsg);
}

void AMainFrame::SetArtTimer(DWORD milliseconds)
{
   CString text;
   text.FormatMessage(rStrTimer, (LPCTSTR)Utils::FormatDuration(milliseconds / 1000));
   m_wndIconBar.fTimer.SetWindowText(text);
   fTimer = milliseconds;
}

void AMainFrame::OnUpdateViewRuler(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(theApp.fShowRuler);
}

HMENU AMainFrame::NewMenu()
{
   static UINT toolbars[] = {rTbMain, rTbFormat};

   // Load the menu from the resources
   fMenu.LoadMenu(rMainFrame);

   // One method for adding bitmaps to menu options is 
   // through the LoadToolbars member function.This method 
   // allows you to add all the bitmaps in a toolbar object 
   // to menu options (if they exist). The first function 
   // parameter is an array of toolbar id's. The second is 
   // the number of toolbar id's. There is also a function 
   // called LoadToolbar that just takes an id.
   fMenu.LoadToolbars(toolbars, _countof(toolbars));

   return fMenu.Detach();
}

HMENU AMainFrame::NewDefaultMenu()
{
   fDefaultMenu.LoadMenu(rMainFrame);
   fDefaultMenu.LoadToolbar(rTbMain);
   return fDefaultMenu.Detach();
}

void AMainFrame::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
   bool setflag = false;
   if (lpMeasureItemStruct->CtlType == ODT_MENU)
   {
      if (IsMenu((HMENU)lpMeasureItemStruct->itemID))
      {
         CMenu* cmenu = CMenu::FromHandle((HMENU)lpMeasureItemStruct->itemID);
         if (fMenu.IsMenu(cmenu) || fDefaultMenu.IsMenu(cmenu))
         {
            fMenu.MeasureItem(lpMeasureItemStruct);
            setflag = true;
         }
      }
   }

   if (!setflag)
      CMDIFrameWnd::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

LRESULT AMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
   if (fMenu.IsMenu(pMenu) || fDefaultMenu.IsMenu(pMenu))
      return BCMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
   else
      return CMDIFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
}

void AMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
   CMDIFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
   if (!bSysMenu && (fMenu.IsMenu(pPopupMenu) || fDefaultMenu.IsMenu(pPopupMenu)))
      BCMenu::UpdateMenu(pPopupMenu);
}

static AMswView* GetActiveView(AMainFrame* frame)
{
   if (NULL == frame)
      return NULL;
   else if (NULL == frame->MDIGetActive())
      return NULL;
   else
      return dynamic_cast<AMswView*>(
         frame->MDIGetActive()->GetActiveView());
}

void AMainFrame::OnUpdateFormatCommand(CCmdUI *pCmdUI)
{  // for some reason, this isn't disabled when previewing, so fix that
   AMswView* view = ::GetActiveView(this);
   pCmdUI->Enable(view && !view->m_bInPrint);
}
