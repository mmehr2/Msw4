// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include <afxtempl.h>

#include "options.h"
#include "ScriptQueue.h"
#include "spellchecker.h"
#include "style.h"
#include "doctype.h"
#include "ScriptQueueDlg.h"
#include "HandController.h"
#include "Comm.h"
#ifdef REGISTER
#  include "RegGen\Registration.h"
#endif // REGISTER

#include <fstream>

class AMswApp : public CWinApp
{
public:
	AMswApp();
	~AMswApp();

	int GetUnits() const {return m_nUnits;}
	int GetTPU() const { return GetTPU(m_nUnits);}
	int GetTPU(int n) const { return m_units[n].m_nTPU;}
	LPCTSTR GetAbbrev() const { return m_units[m_nUnits].m_strAbbrev;}
	LPCTSTR GetAbbrev(int n) const { return m_units[n].m_strAbbrev;}
	const AUnit& GetUnit() const {return m_units[m_nUnits];}

	void SetUnits(int n) {ASSERT((n >= 0) && (n < kNumPrimaryNumUnits)); m_nUnits = n;}

	void NotifyPrinterChanged(BOOL bUpdatePrinterSelection = FALSE);
	BOOL PromptForFileName(CString& fileName, UINT nIDSTitle, DWORD dwFlags,
		bool bOpenFileDialog, DocType::Id* pType = NULL);

	BOOL ParseMeasurement(TCHAR* buf, int& lVal);
	void SaveOptions();
	void LoadOptions();
	void LoadAbbrevStrings();
	HGLOBAL CreateDevNames();
   void SetScrollMargins(int left, int right);

	BOOL IsIdleMessage(MSG* pMsg);

   CString GetExeDir() const;
   void ShowHelp(UINT uCommand, DWORD_PTR dwData = 0);
   void ShowWhatsThisHelp(const DWORD* ids, HANDLE control);
   bool GetProfileBinary(LPCTSTR key, void* bytes, size_t size);

   //--------------------------------------------------------------------------
   // copy protection
   enum {kLegalCheckInterval = 1000 * 60};// * 10}; // 10 minutes
   bool InitProtection();
   bool IsLegalCopy();
   void SetRegistration(LPCTSTR name, LPCTSTR number);

   void DetectProblematicProcesses();

	//{{AFX_VIRTUAL(AMswApp)
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

public:  // data
	static UINT sOpenMsg;
	static UINT sPrinterChangedMsg;
   static UINT sShowRulerMsg;
   static UINT sSetScrollMarginsMsg;
   static UINT sRedrawCueBarMsg;
   static UINT sInverseEditModeMsg;
	static UINT sSetScrollSpeedMsg;
	static UINT sSetScrollPosMsg;
	static UINT sSetScrollPosSyncMsg;

   CDC m_dcScreen;
	LOGFONT m_lf;
	int m_nDefFont;
	CList<HWND, HWND> m_listPrinterNotify;
   BOOL fWordSel;
	CRect m_rectPageMargin;
	WINDOWPLACEMENT m_windowPlacement;
	WINDOWPLACEMENT m_scrollWindowPlacement;
	WINDOWPLACEMENT m_scriptQWindowPlacement;
	DocType::Id m_nNewDocType;
	BOOL m_bForceOEM;
   bool m_bVoiceActivated;
	BOOL m_bMaximized;
   bool fLineNumbers;
   bool fResetTimer;
   bool fManualTimer;
   bool fTimerOn;
   bool fClearPaperclips;
   bool fInverseEditMode;
   CString fFontLanguage;
   BYTE fCuePointer;

   enum {kScroll, kNextBookmark, kPrevBookmark, kSetPaperclip, kGotoPaperclip, kNone};
   int fLButtonAction;
   int fRButtonAction;
   bool fReverseSpeedControl;
   bool fViewScriptQueue;
   AScriptQueue fScriptQueue;
   AScriptQueueDlg fScriptQueueDlg;
   AHandController fHandController;
   CCharFormat fDefaultFont;
   bool fShowRuler;
   int fScrollMarginLeft;
   int fScrollMarginRight;

   static const int kDefaultCharsPerSecond = 172;
   int fCharsPerSecond;

   // video options
   bool fColorOut, fInverseOut, fShowDividers, fShowSpeed, fShowPos, fShowTimer,
      fLoop, fLink, fMirror;

#ifdef REGISTER
   ARegistration fReg;
#endif // REGISTER

private: // methods
	//{{AFX_MSG(AMswApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
   afx_msg void OnBetaFeedback();
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private: // data
   CCommandLineInfo m_cmdInfo;
	int m_nFilterIndex;
   CString m_strHelpFilePath;
	int m_nUnits;
   enum {kNumUnits = 7, kNumPrimaryNumUnits = 4};
	static AUnit m_units[kNumUnits];
};

extern AMswApp theApp;
extern AStyleList gStyles;
extern CSpellchecker1 gSpeller;
extern std::ofstream gLog;
extern AComm gComm;

#define WMA_UPDATE_STYLES		(WM_APP+1)
#define WMA_UPDATE_SETTINGS		(WM_APP+2)
#define WMA_UPDATE_STATUS		(WM_APP+3)
#define WMA_UPDATE_CONTROLS		(WM_APP+4)
