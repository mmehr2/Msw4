// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"

#include "Msw.h"
#include "childfrm.h"
#include "key.h"
#include "mainfrm.h"
#include "mswdoc.h"
#include "mswview.h"
#include "splash.h"
#include "strings.h"
#include "style.h"
#include "EvalDialog.h"
#include "ScrollDialog.h"

#include <locale.h>
#include <winnls.h>
#include <winreg.h>
#include <psapi.h>
#include <tlhelp32.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static LPCTSTR kRichDll = _T("RICHED20.DLL");

//-----------------------------------------------------------------------------
// config file
static LPCTSTR kConfigFile = _T("./Config.txt");
static LPCTSTR kProcesses = _T("Problematic Processes");

AMswApp theApp;
AStyleList gStyles;
CSpellchecker1 gSpeller;
std::ofstream gLog("MagicScroll Log.txt");
#ifdef _REMOTE
   AComm gComm;
#endif // _REMOTE

static ASplashWnd gSplash(true);

CLIPFORMAT cfRTF;

UINT AMswApp::sOpenMsg               = RegisterWindowMessage(_T("MswOpenMessage"));
UINT AMswApp::sPrinterChangedMsg     = RegisterWindowMessage(_T("MswPrinterChanged"));
UINT AMswApp::sShowRulerMsg          = RegisterWindowMessage(_T("MswShowRuler"));
UINT AMswApp::sSetScrollMarginsMsg   = RegisterWindowMessage(_T("MswSetScrollMargins"));
UINT AMswApp::sRedrawCueBarMsg       = RegisterWindowMessage(_T("MswRedrawCueBar"));
UINT AMswApp::sInverseEditModeMsg    = RegisterWindowMessage(_T("MswInverseEditMode"));
UINT AMswApp::sSetScrollSpeedMsg     = RegisterWindowMessage(_T("MswSetScrollSpeed"));
UINT AMswApp::sSetScrollPosMsg       = RegisterWindowMessage(_T("MswSetScrollPos"));
UINT AMswApp::sSetScrollPosSyncMsg   = RegisterWindowMessage(_T("MswSetScrollPosSync"));

AUnit AMswApp::m_units[] = {
   //  TPU,    SmallDiv,   MedDiv, LargeDiv,   MinMove,    szAbbrev,           bSpace
   AUnit(1440, 180,        720,    1440,       90,         IDS_INCH1_ABBREV,   FALSE),//inches
   AUnit(568,  142,        284,    568,        142,        IDS_CM_ABBREV,      TRUE),//centimeters
   AUnit(20,   120,        720,    720,        100,        IDS_POINT_ABBREV,   TRUE),//points
   AUnit(240,  240,        1440,   1440,       120,        IDS_PICA_ABBREV,    TRUE),//picas
   AUnit(1440, 180,        720,    1440,       90,         IDS_INCH2_ABBREV,   FALSE),//in
   AUnit(1440, 180,        720,    1440,       90,         IDS_INCH3_ABBREV,   FALSE),//inch
   AUnit(1440, 180,        720,    1440,       90,         IDS_INCH4_ABBREV,   FALSE)//inches
};

BEGIN_MESSAGE_MAP(AMswApp, CWinApp)
	//{{AFX_MSG_MAP(AMswApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
   ON_COMMAND(rCmdBetaFeedback, OnBetaFeedback)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

AMswApp::AMswApp() : 
	m_nDefFont(DEFAULT_GUI_FONT),
	m_nFilterIndex(2),  // default to RTF files
	m_bForceOEM(FALSE),
   m_bVoiceActivated(true),
   m_bMaximized(FALSE),
   fLineNumbers(true),
   fResetTimer(false),
   fManualTimer(true),
   fTimerOn(true),
   fClearPaperclips(true),
   fLButtonAction(kScroll),
   fRButtonAction(kNextBookmark),
   fReverseSpeedControl(false),
   fViewScriptQueue(false),
   // video options
   fColorOut(true),
   fInverseOut(true),
   fShowDividers(true),
   fShowSpeed(true),
   fShowPos(true),
   fShowTimer(true),
   fLoop(false),
   fLink(true),
   fMirror(false),

   fCharsPerSecond(kDefaultCharsPerSecond),
   fShowRuler(true),
   fScrollMarginLeft(0),
   fScrollMarginRight(600),
   fInverseEditMode(false),
   fFontLanguage("Western"),
   fCuePointer(0)
{
	_tsetlocale(LC_ALL, _T(""));

	m_dcScreen.Attach(::GetDC(NULL));

   ::memset(&m_windowPlacement, 0, sizeof(m_windowPlacement));
   ::memset(&m_scrollWindowPlacement, 0, sizeof(m_scrollWindowPlacement));
   ::memset(&m_scriptQWindowPlacement, 0, sizeof(m_scriptQWindowPlacement));
   ::memset(&fDefaultFont, 0, sizeof(fDefaultFont));
}

AMswApp::~AMswApp()
{
	if (m_dcScreen.m_hDC != NULL)
		::ReleaseDC(NULL, m_dcScreen.Detach());
}

// Register the application's document templates.  Document templates
//  serve as the connection between documents, frame windows and views.
static CMultiDocTemplate DocTemplate(rMainFrame, RUNTIME_CLASS(AMswDoc), 
   RUNTIME_CLASS(AChildFrame), RUNTIME_CLASS(AMswView));

BOOL AMswApp::InitInstance()
{
   this->SetRegistryKey(gStrRegKey);
	this->ParseCommandLine(m_cmdInfo);

	HWND me = ::FindWindow(gStrMswClass, NULL);
	if (NULL != me)
   {  // MagicScroll is already running - bring it forward and quit
      ::ShowWindow(me, SW_SHOW);
      ::SetForegroundWindow(me);
		return FALSE;
   }

   if (!this->InitProtection())
   {
      ::AfxMessageBox(rStrInitProtection);
      return FALSE;
   }
   else if (!this->IsLegalCopy())
      return FALSE;

   VERIFY(NULL != ::LoadLibrary(kRichDll));
	this->LoadOptions();

	// Initialize OLE libraries
   if (!::AfxOleInit())
	{
      ::AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	BOOL bSplash = m_cmdInfo.m_bShowSplash;
	switch (m_nCmdShow)
	{
		case SW_HIDE:
		case SW_SHOWMINIMIZED:
		case SW_MINIMIZE:
		case SW_SHOWMINNOACTIVE:
		bSplash = FALSE;
		break;

		case SW_RESTORE:
		case SW_SHOW:
		case SW_SHOWDEFAULT:
		case SW_SHOWNA:
		case SW_SHOWNOACTIVATE:
		case SW_SHOWNORMAL:
		case SW_SHOWMAXIMIZED:
		if (m_bMaximized)
		   m_nCmdShow = SW_SHOWMAXIMIZED;
	   break;
	}

#ifndef _DEBUG
   if (bSplash)
	{
		gSplash.Create(NULL);
		gSplash.ShowWindow(SW_SHOW);
		gSplash.UpdateWindow();
	}
#endif // _DEBUG

	this->LoadAbbrevStrings();

	m_hDevNames = this->CreateDevNames();
	this->NotifyPrinterChanged((m_hDevNames == NULL));

	free((void*)m_pszHelpFilePath);
	m_strHelpFilePath = this->GetExeDir() + _T("\\Msw.chm");
	m_pszHelpFilePath = m_strHelpFilePath;

	cfRTF = (CLIPFORMAT)::RegisterClipboardFormat(CF_RTFNOOBJS);

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	this->LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	DocTemplate.SetContainerInfo(rMenuInPlace);

	// create main MDI Frame window
	AMainFrame* pMainFrame = new AMainFrame;
	if (!pMainFrame->LoadFrame(rMainFrame))
		return FALSE;
	m_pMainWnd = pMainFrame;

#ifdef _REMOTE
   // kick off remote communications
   gComm.ConfigureSettings();
   gComm.SetParent(m_pMainWnd->m_hWnd);
   // MLM: For pubnub to save money - don't connect until login on RemoteDialog
   //gComm.Connect(gComm.GetUsername(), gComm.GetPassword());
#endif // _REMOTE

   // This code replaces the MFC created menus with 
   // the Ownerdrawn versions 
   DocTemplate.m_hMenuShared = pMainFrame->NewMenu();
   pMainFrame->m_hMenuDefault = pMainFrame->NewDefaultMenu();

   // This simulates a window being opened if you don't have
   // a default window displayed at startup
   pMainFrame->OnUpdateFrameMenu(pMainFrame->m_hMenuDefault);

	// Dispatch commands specified on the command line
   BOOL docOpened = FALSE;
	if (m_cmdInfo.m_nShellCommand != CCommandLineInfo::FileNew)
      docOpened = ProcessShellCommand(m_cmdInfo);

	// Enable File Manager drag/drop open
	m_pMainWnd->DragAcceptFiles();

   // create and initialize the spell checker
   CRect r;
   if (gSpeller.Create(NULL, WS_OVERLAPPED, r, m_pMainWnd, 0, 0, 0, 0))
   {  // load saved settings
      gSpeller.put_CheckSpellingAsYouType(GetProfileInt(gStrSpeller, gStrAsYouType, TRUE));
      gSpeller.put_AlwaysSuggest(GetProfileInt(gStrSpeller, gStrAlwaysSuggest, TRUE));
      gSpeller.put_SuggestFromMainDictionariesOnly(GetProfileInt(gStrSpeller, gStrFromMain, FALSE));
      gSpeller.put_IgnoreWordsInUppercase(GetProfileInt(gStrSpeller, gStrIgnoreUpper, TRUE));
      gSpeller.put_IgnoreWordsWithNumbers(GetProfileInt(gStrSpeller, gStrIgnoreNumbers, TRUE));
      gSpeller.put_IgnoreInternetAndFileAddresses(GetProfileInt(gStrSpeller, gStrIgnoreLinks, TRUE));

      // auto correct
      gSpeller.put_CorrectTwoInitCaps(GetProfileInt(gStrSpeller, gStrCorrect2Caps, TRUE));
      gSpeller.put_CapitalizeSentence(GetProfileInt(gStrSpeller, gStrCapFirst, TRUE));
      gSpeller.put_CorrectCapsLock(GetProfileInt(gStrSpeller, gStrCapsLock, TRUE));
      gSpeller.put_AutoCorrectTextAsYouType(GetProfileInt(gStrSpeller, gStrReplaceText, TRUE));
      gSpeller.put_AutoAddWordsToExceptionList(GetProfileInt(gStrSpeller, gStrAddWords, TRUE));

      CString strExeDir = theApp.GetExeDir();
      if (Utils::FileExists(strExeDir + gStrAutoCorrect))
		   gSpeller.InitializeAutoCorrection(strExeDir + gStrAutoCorrect);

      // enable the 'change' menu when right clicking on RichTextControl
      gSpeller.put_ChangePopup(TRUE);

      // load dictionaries
      CString key;
      int nDictsLoaded = 0;
      for (long i = 0; ; ++i)
      {
         key.Format(_T("%s%d"), (LPCTSTR)gStrDict, i);
         CString dict = this->GetProfileString(gStrSpeller, key);
         if (!dict.IsEmpty() && Utils::FileExists(dict))
         {
            gSpeller.OpenDictionary(dict);
            ++nDictsLoaded;
         }
         else
            break;
      }
      for (long i = 0; ; ++i)
      {
         key.Format(_T("%s%d"), (LPCTSTR)gStrCustomDict, i);
         CString dict = this->GetProfileString(gStrSpeller, key);
         if (!dict.IsEmpty())
            gSpeller.OpenCustomDictionary(dict);
         else
            break;
      }
      // if we didn't load any dictionaries, load English by default
      if ((0 == nDictsLoaded) && Utils::FileExists(strExeDir + gStrDefaultDict))
	      gSpeller.OpenDictionary(strExeDir + gStrDefaultDict);

   }
   else
   {
      ::AfxMessageBox(rStrSpellerInit);
   }

	if (m_pMainWnd == NULL) // i.e. OnFileNew failed
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

   if (fViewScriptQueue)
   {  // the handler for this meesage will toggle the flag, so flip it again here
      fViewScriptQueue = false;
      this->m_pMainWnd->PostMessage(WM_COMMAND, rCmdViewQueue);
   }

   if (!docOpened)
   {  // open the last documents opened
      CString key;
      int openedFiles = 0;
      for (int i = 0; ; ++i)
      {
         key.Format(_T("%s%d"), (LPCTSTR)gStrLastFile, i);
         CString path = this->GetProfileString(gStrSection, key);
         if (!path.IsEmpty())
            openedFiles += (NULL != this->OpenDocumentFile(path));
         else
            break;
      }

      if (openedFiles > 0)
         theApp.fScriptQueue.Activate(0);
      else
	      this->OnFileNew();
   }

   { // diagnostics - create a log file
      // Get all modules loaded by this process
      gLog << "Loaded Modules: ";
      DWORD dwModuleSize = 0;
      HMODULE hModules[1000] = {0};
      const HANDLE hProcess = ::GetCurrentProcess();
      VERIFY(::EnumProcessModules(hProcess, hModules, sizeof(hModules), &dwModuleSize));
      for (DWORD i = 0; i < (dwModuleSize / sizeof(HMODULE)); ++i)
      {  // Fill out process information
         gLog << std::endl << ' ';
         char buffer[MAX_PATH] = {0};
         if (::GetModuleBaseNameA(hProcess, hModules[i], buffer, _countof(buffer)))
            gLog << buffer << ',';


         DWORD dwDummy = 0;
         DWORD dwSize  = ::GetFileVersionInfoSizeA(buffer, &dwDummy);
         if (dwSize > 0)
         {		
            UINT uLen = 0;
            LPVOID lpVSFFI = NULL;
            LPVOID lpBuffer = (LPBYTE)malloc(dwSize);
            if (::GetFileVersionInfoA(buffer, 0, dwSize, lpBuffer))
            {
               if (::VerQueryValueA(lpBuffer, "\\", &lpVSFFI, &uLen))
               {
                  VS_FIXEDFILEINFO info = {0};
                  ::CopyMemory(&info, lpVSFFI, sizeof(VS_FIXEDFILEINFO));
                  ASSERT(info.dwSignature == VS_FFI_SIGNATURE);
                  WORD M = HIWORD(info.dwFileVersionMS);
                  WORD m = LOWORD(info.dwFileVersionMS);
                  WORD b = HIWORD(info.dwFileVersionLS);
                  gLog << M << '.' << m << '.' << b << ',';
               }
            }
         }

         if (::GetModuleFileNameExA(hProcess, hModules[i], buffer, _countof(buffer)))
            gLog << buffer;
      }
   }

   this->DetectProblematicProcesses();

   return TRUE;
}

void AMswApp::SaveOptions()
{
   this->WriteProfileInt(gStrSection, gStrWordSel, fWordSel);
	this->WriteProfileInt(gStrSection, gStrUnits, GetUnits());
	this->WriteProfileInt(gStrSection, gStrMaximized, m_bMaximized);
   this->WriteProfileInt(gStrSection, gStrVoiceActivated, m_bVoiceActivated);
	this->WriteProfileBinary(gStrSection, gStrFrameRect, (BYTE*)&m_windowPlacement, sizeof(m_windowPlacement));
	this->WriteProfileBinary(gStrSection, gStrScrollFrameRect, (BYTE*)&m_scrollWindowPlacement, sizeof(m_scrollWindowPlacement));
	this->WriteProfileBinary(gStrSection, gStrScriptQRect, (BYTE*)&m_scriptQWindowPlacement, sizeof(m_scriptQWindowPlacement));
   this->WriteProfileBinary(gStrSection, gStrPageMargin, (BYTE*)&m_rectPageMargin, sizeof(CRect));
   this->WriteProfileBinary(gStrSection, gStrDefaultFont, (BYTE*)&fDefaultFont, sizeof(fDefaultFont));
	this->WriteProfileInt(gStrSection, gStrLineNumbers, fLineNumbers);
	this->WriteProfileInt(gStrSection, gStrResetTimer, fResetTimer);
	this->WriteProfileInt(gStrSection, gStrManualTimer, fManualTimer);
	this->WriteProfileInt(gStrSection, gStrTimerOn, fTimerOn);
	this->WriteProfileInt(gStrSection, gStrClearPaperclips, fClearPaperclips);
	this->WriteProfileInt(gStrSection, gStrLButtonAction, fLButtonAction);
	this->WriteProfileInt(gStrSection, gStrRButtonAction, fRButtonAction);
	this->WriteProfileInt(gStrSection, gStrReverseSpeedControl, fReverseSpeedControl);
   this->WriteProfileInt(gStrSection, gStrViewScriptQueue, fViewScriptQueue);
   this->WriteProfileInt(gStrSection, gStrCharsPerSecond, fCharsPerSecond);
   this->WriteProfileInt(gStrSection, gStrShowRuler, fShowRuler);
   this->WriteProfileInt(gStrSection, gStrScrollMarginLeft, fScrollMarginLeft);
   this->WriteProfileInt(gStrSection, gStrScrollMarginRight, fScrollMarginRight);
	this->WriteProfileInt(gStrSection, gStrInverseEditVideo, fInverseEditMode);
	this->WriteProfileString(gStrSection, gStrFontLanguage, fFontLanguage);
	this->WriteProfileInt(gStrSection, gStrCuePointer, fCuePointer);

   // video options
   this->WriteProfileInt(gStrSection, gStrColorOut, fColorOut);
   this->WriteProfileInt(gStrSection, gStrInverseOut, fInverseOut);
   this->WriteProfileInt(gStrSection, gStrShowDividers, fShowDividers);
   this->WriteProfileInt(gStrSection, gStrShowSpeed, fShowSpeed);
   this->WriteProfileInt(gStrSection, gStrShowPos, fShowPos);
   this->WriteProfileInt(gStrSection, gStrShowTimer, fShowTimer);
   this->WriteProfileInt(gStrSection, gStrLoop, fLoop);
   this->WriteProfileInt(gStrSection, gStrLink, fLink);
   this->WriteProfileInt(gStrSection, gStrMirror, fMirror);
   this->WriteProfileInt(gStrSection, gStrFrameInterval, AScrollDialog::gMinFrameInterval);

   VERIFY(gStyles.Write());
   //fCaption.SaveCCSettings();
#ifdef _REMOTE
   gComm.Write();
#endif // _REMOTE
}

bool AMswApp::GetProfileBinary(LPCTSTR key, void* bytes, size_t size)
{
	BYTE* pb = NULL;
	UINT nLen = 0;
   if (theApp.CWinApp::GetProfileBinary(gStrSection, key, &pb, &nLen))
	{
		ASSERT(nLen == size);
		memcpy(bytes, pb, size);
		delete pb;
      return true;
	}

   return false;
}

void AMswApp::LoadOptions()
{
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	VERIFY(GetObject(hFont, sizeof(LOGFONT), &m_lf));

	fWordSel = GetProfileInt(gStrSection, gStrWordSel, TRUE);
   TCHAR buf[2] = {0};
	GetLocaleInfo(GetUserDefaultLCID(), LOCALE_IMEASURE, buf, 2);
	int nDefUnits = buf[0] == '1' ? 0 : 1;
	SetUnits(GetProfileInt(gStrSection, gStrUnits, nDefUnits));
	m_bMaximized = GetProfileInt(gStrSection, gStrMaximized, (int)FALSE);
   m_bVoiceActivated = this->GetProfileInt(gStrSection, gStrVoiceActivated, true) ? true : false;

   this->GetProfileBinary(gStrFrameRect, &m_windowPlacement, sizeof(m_windowPlacement));
   this->GetProfileBinary(gStrScrollFrameRect, &m_scrollWindowPlacement, sizeof(m_scrollWindowPlacement));
   this->GetProfileBinary(gStrScriptQRect, &m_scriptQWindowPlacement, sizeof(m_scriptQWindowPlacement));
   if (!this->GetProfileBinary(gStrPageMargin, &m_rectPageMargin, sizeof(m_rectPageMargin)))
		m_rectPageMargin.SetRect(1800, 1440, 1800, 1440);
   if (!this->GetProfileBinary(gStrDefaultFont, &fDefaultFont, sizeof(fDefaultFont)))
   {  // set the default font
      ::memset(&fDefaultFont, 0, sizeof(fDefaultFont));
	   fDefaultFont.cbSize = sizeof(fDefaultFont);
	   CString name;
	   VERIFY(name.LoadString(rStrDefaultFont));
	   fDefaultFont.dwMask = CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE|CFM_STRIKEOUT|CFM_SIZE|
		   CFM_COLOR|CFM_OFFSET|CFM_PROTECTED;
	   fDefaultFont.dwEffects = CFE_AUTOCOLOR;
	   fDefaultFont.yHeight = 1280; // 64pt
	   fDefaultFont.yOffset = 0;
	   fDefaultFont.crTextColor = RGB(0, 0, 0);
	   fDefaultFont.bCharSet = 0;
	   fDefaultFont.bPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	   ASSERT(name.GetLength() < LF_FACESIZE);
      ::_tcsncpy_s(fDefaultFont.szFaceName, mCountOf(fDefaultFont.szFaceName), (LPTSTR)(LPCTSTR)name, LF_FACESIZE);
	   fDefaultFont.dwMask |= CFM_FACE;
   }

   fLineNumbers = GetProfileInt(gStrSection, gStrLineNumbers, fLineNumbers) ? true : false;
   fClearPaperclips = GetProfileInt(gStrSection, gStrClearPaperclips, fClearPaperclips) ? true : false;
   fResetTimer = GetProfileInt(gStrSection, gStrResetTimer, fResetTimer) ? true : false;
   fManualTimer = GetProfileInt(gStrSection, gStrManualTimer, fManualTimer) ? true : false;
   fTimerOn = GetProfileInt(gStrSection, gStrTimerOn, fTimerOn) ? true : false;
	fLButtonAction = GetProfileInt(gStrSection, gStrLButtonAction, fLButtonAction);
	fRButtonAction = GetProfileInt(gStrSection, gStrRButtonAction, fRButtonAction);
   fReverseSpeedControl = GetProfileInt(gStrSection, gStrReverseSpeedControl, fReverseSpeedControl) ? true : false;
   fViewScriptQueue = GetProfileInt(gStrSection, gStrViewScriptQueue, fViewScriptQueue) ? true : false;
   fCharsPerSecond = GetProfileInt(gStrSection, gStrCharsPerSecond, fCharsPerSecond);
   fShowRuler = GetProfileInt(gStrSection, gStrShowRuler, fShowRuler) ? true : false;
   fScrollMarginLeft = GetProfileInt(gStrSection, gStrScrollMarginLeft, fScrollMarginLeft);
   fScrollMarginRight = GetProfileInt(gStrSection, gStrScrollMarginRight, fScrollMarginRight);
   fInverseEditMode = GetProfileInt(gStrSection, gStrInverseEditVideo, fInverseEditMode) ? true : false;
   fFontLanguage = GetProfileString(gStrSection, gStrFontLanguage, fFontLanguage);
   fCuePointer = (BYTE)GetProfileInt(gStrSection, gStrCuePointer, fCuePointer);

   // video options
   fColorOut = this->GetProfileInt(gStrSection, gStrColorOut, fColorOut) ? true : false;
   fInverseOut = this->GetProfileInt(gStrSection, gStrInverseOut, fInverseOut) ? true : false;
   fShowDividers = this->GetProfileInt(gStrSection, gStrShowDividers, fShowDividers) ? true : false;
   fShowSpeed = this->GetProfileInt(gStrSection, gStrShowSpeed, fShowSpeed) ? true : false;
   fShowPos = this->GetProfileInt(gStrSection, gStrShowPos, fShowPos) ? true : false;
   fShowTimer = this->GetProfileInt(gStrSection, gStrShowTimer, fShowTimer) ? true : false;
   fLoop = this->GetProfileInt(gStrSection, gStrLoop, fLoop) ? true : false;
   fLink = this->GetProfileInt(gStrSection, gStrLink, fLink) ? true : false;
   fMirror = this->GetProfileInt(gStrSection, gStrMirror, fMirror) ? true : false;
   AScrollDialog::gMinFrameInterval = this->GetProfileInt(
      gStrSection, gStrFrameInterval, AScrollDialog::gMinFrameInterval);

   VERIFY(gStyles.Read());
   //fCaption.LoadCCSettings();
#ifdef _REMOTE
   gComm.Read();
#endif // _REMOTE
}

void AMswApp::LoadAbbrevStrings()
{
   for (int i = 0; i < mCountOf(m_units); ++i)
		m_units[i].m_strAbbrev.LoadString(m_units[i].m_nAbbrevID);
}

BOOL AMswApp::ParseMeasurement(LPTSTR buf, int& lVal)
{
	if (buf[0] == NULL)
		return FALSE;

	TCHAR* pch = NULL;
   float f = (float)_tcstod(buf,&pch);

	// eat white space, if any
	while (isspace(*pch))
		pch++;

	if (pch[0] == NULL) // default
	{
		lVal = (f < 0.f) ? (int)(f*GetTPU()-0.5f) : (int)(f*GetTPU()+0.5f);
		return TRUE;
	}
	for (int i = 0; i < mCountOf(m_units); ++i)
	{
		if (lstrcmpi(pch, GetAbbrev(i)) == 0)
		{
			lVal = (f < 0.f) ? (int)(f*GetTPU(i)-0.5f) : (int)(f*GetTPU(i)+0.5f);
			return TRUE;
		}
	}
	return FALSE;
}

void AMswApp::OnAppAbout()
{
	ASplashWnd().DoModal();
}

int AMswApp::ExitInstance()
{
	m_pszHelpFilePath = NULL;

   ::FreeLibrary(GetModuleHandle(kRichDll));
	this->SaveOptions();

   fHandController.Disconnect();
   gSplash.DestroyWindow();
   fScriptQueueDlg.DestroyWindow();
   gSpeller.DestroyWindow();

   gLog.close();

#ifdef _REMOTE
   gComm.Disconnect();
   gComm.SetParent(NULL);
#endif // _REMOTE

	return CWinApp::ExitInstance();
}

void AMswApp::OnFileNew()
{
	m_nNewDocType = DocType::RD_DEFAULT;
   this->OpenDocumentFile(NULL);
}

// prompt for file name - used for open and save as
// static function called from app
BOOL AMswApp::PromptForFileName(CString& fileName, UINT nIDSTitle,
	DWORD dwFlags, bool bOpenFileDialog, DocType::Id* pType)
{
	ScanForConverters();
	CFileDialog dlgFile(bOpenFileDialog);
	dlgFile.m_ofn.Flags |= dwFlags;

   CString title;
	VERIFY(title.LoadString(nIDSTitle));

	int nIndex = m_nFilterIndex;
	if (!bOpenFileDialog)
	{
		DocType::Id nDocType = (pType != NULL) ? *pType : DocType::RD_DEFAULT;
		nIndex = GetIndexFromType(nDocType, bOpenFileDialog);
		if (nIndex == -1)
			nIndex = GetIndexFromType(DocType::RD_DEFAULT, bOpenFileDialog);
		if (nIndex == -1)
         nIndex = GetIndexFromType(DocType::RD_NATIVE, bOpenFileDialog);
		ASSERT(nIndex != -1);
		nIndex++;
	}
	dlgFile.m_ofn.nFilterIndex = nIndex;
	// strDefExt is necessary to hold onto the memory from GetExtFromType
	CString strDefExt = GetExtFromType(GetTypeFromIndex(nIndex-1, bOpenFileDialog));
	dlgFile.m_ofn.lpstrDefExt = strDefExt;


	CString strFilter = GetFileTypes(bOpenFileDialog);
	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);

	BOOL bRet = (dlgFile.DoModal() == IDOK) ? TRUE : FALSE;
	fileName.ReleaseBuffer();
	if (bRet)
	{
		if (bOpenFileDialog)
			m_nFilterIndex = dlgFile.m_ofn.nFilterIndex;
		if (pType != NULL)
		{
			int nIndex = (int)dlgFile.m_ofn.nFilterIndex - 1;
			ASSERT(nIndex >= 0);
			*pType = GetTypeFromIndex(nIndex, bOpenFileDialog);
		}
	}
	return bRet;
}

void AMswApp::OnFileOpen()
{
	// prompt the user (with all document templates)
	CString newName;
	DocType::Id nType = DocType::RD_DEFAULT;
	if (!PromptForFileName(newName, AFX_IDS_OPENFILE, OFN_HIDEREADONLY | 
      OFN_FILEMUSTEXIST, true, &nType))
		return; // open cancelled

	if (nType == DocType::RD_OEMTEXT)
		m_bForceOEM = TRUE;
	this->OpenDocumentFile(newName);
	m_bForceOEM = FALSE;
	// if returns NULL, the user has already been alerted
}

void AMswApp::WinHelp(DWORD dwData, UINT nCmd)
{
   switch (nCmd)
   {
      case HELP_CONTEXT:   HtmlHelp(HH_HELP_CONTEXT, dwData); break;
      case HELP_FINDER:    HtmlHelp(HH_DISPLAY_TOPIC, 0); break;	
   }
}

BOOL AMswApp::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_PAINT)
		return FALSE;

   return CWinThread::PreTranslateMessage(pMsg);
}

void AMswApp::NotifyPrinterChanged(BOOL bUpdatePrinterSelection)
{
	if (bUpdatePrinterSelection)
		UpdatePrinterSelection(FALSE);
	POSITION pos = m_listPrinterNotify.GetHeadPosition();
	while (pos != NULL)
	{
		HWND hWnd = m_listPrinterNotify.GetNext(pos);
		::SendMessage(hWnd, sPrinterChangedMsg, 0, 0);
	}
}

BOOL AMswApp::IsIdleMessage(MSG* pMsg)
{
	if (pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_NCMOUSEMOVE)
		return FALSE;
	return CWinApp::IsIdleMessage(pMsg);
}

HGLOBAL AMswApp::CreateDevNames()
{
	HGLOBAL hDev = NULL;
	if (!m_cmdInfo.m_strDriverName.IsEmpty() && !m_cmdInfo.m_strPrinterName.IsEmpty() &&
		!m_cmdInfo.m_strPortName.IsEmpty())
	{
		hDev = GlobalAlloc(GPTR, 4*sizeof(WORD)+
			m_cmdInfo.m_strDriverName.GetLength() + 1 +
			m_cmdInfo.m_strPrinterName.GetLength() + 1 +
			m_cmdInfo.m_strPortName.GetLength()+1);
		LPDEVNAMES lpDev = (LPDEVNAMES)GlobalLock(hDev);
		lpDev->wDriverOffset = sizeof(WORD)*4;
		lstrcpy((TCHAR*)lpDev + lpDev->wDriverOffset, m_cmdInfo.m_strDriverName);

		lpDev->wDeviceOffset = (WORD)(lpDev->wDriverOffset + m_cmdInfo.m_strDriverName.GetLength()+1);
		lstrcpy((TCHAR*)lpDev + lpDev->wDeviceOffset, m_cmdInfo.m_strPrinterName);

		lpDev->wOutputOffset = (WORD)(lpDev->wDeviceOffset + m_cmdInfo.m_strPrinterName.GetLength()+1);
		lstrcpy((TCHAR*)lpDev + lpDev->wOutputOffset, m_cmdInfo.m_strPortName);

		lpDev->wDefault = 0;
	}
	return hDev;
}

CString AMswApp::GetExeDir() const
{
   TCHAR szDir[MAX_PATH] = {0};
   if (::GetModuleFileName(NULL, szDir, mCountOf(szDir) - 1) > 0)
   {
      *::_tcsrchr(szDir, _T('\\')) = 0;
   }
   return szDir;
}

CDocument* AMswApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
   BOOL maximized;
   CMDIChildWnd* cw = ((CMDIFrameWnd*)m_pMainWnd)->MDIGetActive(&maximized);

   CDocument* result = NULL;
   if (NULL == lpszFileName) 
      result = DocTemplate.OpenDocumentFile(NULL);
   else if (!Utils::FileExists(lpszFileName))
      ::AfxMessageBox(AFX_IDP_FAILED_TO_OPEN_DOC);
   else
      result = CWinApp::OpenDocumentFile(lpszFileName);

   if (NULL == cw)
      m_pMainWnd->PostMessage(WM_MDIMAXIMIZE);

   return result;
}

void AMswApp::SetScrollMargins(int left, int right)
{
   if ((fScrollMarginLeft != left) || 
      (fScrollMarginRight != right))
   {
      fScrollMarginLeft = left;
      fScrollMarginRight = right;
      m_pMainWnd->SendMessageToDescendants(AMswApp::sSetScrollMarginsMsg);
   }
}

void AMswApp::OnBetaFeedback()
{
   ::ShellExecute(NULL, _T("open"), _T("mailto:EricS@EcsVideoSystems.com?SUBJECT=MSW 4.0.0 Beta"), 
      NULL, NULL, SW_SHOWNORMAL);
}

void AMswApp::ShowHelp(UINT uCommand, DWORD_PTR dwData)
{
   ASSERT(HH_DISPLAY_TEXT_POPUP != uCommand);   // call ShowWhatsThisHelp instead
   ::HtmlHelp(NULL, m_pszHelpFilePath, uCommand, dwData);
}

void AMswApp::ShowWhatsThisHelp(const DWORD* ids, HANDLE control)
{
   CString path = m_pszHelpFilePath + CString("::/WhatsThisTopics.txt");
   ::HtmlHelp((HWND)control, path, HH_TP_HELP_WM_HELP, (DWORD_PTR)ids);
}

typedef std::pair<CString, DWORD> Process;
typedef std::vector<Process> ProcessList;
static ProcessList gProcesses;
static int EnumerateProcesses()
{  // enumerate all running Win32 processes and save them in 'gProcesses'
   gProcesses.clear();
   HANDLE h = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (INVALID_HANDLE_VALUE != h)
   {
      PROCESSENTRY32 i;
      i.dwSize = sizeof(i);
      for (BOOL b=::Process32First(h, &i); b; b=::Process32Next(h, &i))
      {  // convert to lower case (for comparing) and save
         ::CharLowerBuff(i.szExeFile, lstrlen(i.szExeFile));
         gProcesses.push_back(Process(i.szExeFile, i.th32ProcessID));
      }
      ::CloseHandle(h);
   }

   return gProcesses.size();
}

void AMswApp::DetectProblematicProcesses()
{
   ::EnumerateProcesses();

   ProcessList processes;
   TCHAR name[2048];
   for (int i = 1; ; ++i) {
      CString key;
      key.Format(_T("%d"), i);
      if (::GetPrivateProfileString(kProcesses, key, NULL, name, sizeof(name), kConfigFile) > 0)
      {  // search the process list for this module
         ::CharLowerBuff(name, lstrlen(name));
         for (size_t i = 0; i < gProcesses.size(); ++i)
            if (name == gProcesses[i].first)
               processes.push_back(gProcesses[i]);
      }
      else
         break;
   }

   if (!processes.empty())
   {  // display to user
      CString names;
      for (size_t i = 0; i < processes.size(); ++i)
         names += '\t' + processes[i].first + '\n';

      CString msg;
      msg.FormatMessage(rStrCloseApps, names);
      if (IDYES == ::AfxMessageBox(msg, MB_YESNO))
      {
         CWaitCursor wait;
         for (size_t i = 0; i < processes.size(); ++i)
         {  // attempt to forcefully terminate it
            DWORD exitCode = 0;
            HANDLE h = ::OpenProcess(PROCESS_ALL_ACCESS, 0, processes[i].second);
            if (NULL != h)
            {
               ::GetExitCodeProcess(h, &exitCode);
               ::TerminateProcess(h, exitCode);
            }
         }

         // wait for a bit
         ::Sleep(3000);

         // now see which processes are left
         ::EnumerateProcesses();
         names = _T("");
         for (size_t i = 0; i < processes.size(); ++i)
            if (gProcesses.end() != std::find(gProcesses.begin(), gProcesses.end(), processes[i]))
               names += '\t' + processes[i].first + '\n';
         if (!names.IsEmpty())
         {  // we failed to kill at least one - let the user know
            msg.FormatMessage(rStrCloseApps2, names);
            ::AfxMessageBox(msg, MB_OK);
         }
      }
   }
}
