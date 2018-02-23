// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#ifndef hMainFrm
#define hMainFrm


#include "Msw.h"
#include "FormatBa.h"
#include "ControlPanel.h"
//#include "SRManager.h"
#include "BCMenu.h"

class AMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNCREATE(AMainFrame)

public:
   AMainFrame() : fTimer(0) {}

   void OnScroll();
   void OnInverseOut();
   void OnInverseEdit();
   DWORD GetTimer() const;

	//{{AFX_VIRTUAL(AMainFrame)
	virtual void ActivateFrame(int nCmdShow = -1);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

   HMENU NewMenu();
   HMENU NewDefaultMenu();
	afx_msg void OnDropFiles(HDROP hDropInfo);
   afx_msg void OnPreferences();

public:
	CToolBar    m_wndToolBar;
	AControlPanel m_wndIconBar;
	CStatusBar  m_wndStatusBar;
	AFormatBar  m_wndFormatBar;

protected:  // control bar embedded members
	BOOL CreateToolBar();
	BOOL CreateIconBar();
	BOOL CreateFormatBar();
	BOOL CreateStatusBar();
   afx_msg LRESULT OnMDIMaximize(WPARAM wParam, LPARAM lParam);

protected:
	//{{AFX_MSG(AMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHelpFinder();
	afx_msg void OnCharColor();
	afx_msg void OnFontChange();
	afx_msg void OnDevModeChange(LPTSTR lpDeviceName);
	afx_msg void OnSysColorChange();
	afx_msg void OnDestroy();
//	afx_msg void OnListen();
//	afx_msg void OnVoiceTraining();
//	afx_msg void OnMicSetup();
//	afx_msg void OnUpdateListen(CCmdUI* pCmdUI);
//	afx_msg void OnUpdateVoiceTraining(CCmdUI* pCmdUI);
//	afx_msg void OnUpdateMicSetup(CCmdUI* pCmdUI);
//   afx_msg LRESULT OnVoiceCommand(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnOpenMsg(UINT wParam, LONG lParam);
	afx_msg LRESULT UpdateStyles(WPARAM wParam, LPARAM lParam);
#ifdef _REMOTE
   afx_msg void OnRemote();
#endif // _REMOTE
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   afx_msg void OnUpdateToggleScrollMode(CCmdUI *pCmdUI);
   afx_msg void OnUpdateToggleInverseOut(CCmdUI *pCmdUI);
   afx_msg void OnUpdateToggleInverseEdit(CCmdUI *pCmdUI);
   afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
   afx_msg void OnClose();
   afx_msg void OnWebLink();
   afx_msg void OnShowLineNumbers();
   afx_msg void OnUpdateShowLineNumbers(CCmdUI *pCmdUI);
   afx_msg void OnTimerOn();
   afx_msg void OnUpdateTimerOn(CCmdUI *pCmdUI);
   afx_msg void OnViewScriptQueue();
   afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
   afx_msg void OnUpdateViewScriptQueue(CCmdUI *pCmdUI);
   afx_msg void OnTimer(UINT);
   afx_msg void OnResetTimer();
   afx_msg void OnLink();
   afx_msg void OnLoop();
   afx_msg void OnUpdateLink(CCmdUI *pCmdUI);
   afx_msg void OnUpdateLoop(CCmdUI *pCmdUI);
   afx_msg void OnRegister();
   afx_msg void OnViewRuler();
   afx_msg void OnUpdateViewRuler(CCmdUI *pCmdUI);
   afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
   afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
   afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
   afx_msg void OnUpdateFormatCommand(CCmdUI *pCmdUI);

   void SetArtTimer(DWORD milliseconds);

private:
   //ASRManager fSpeech;
   DWORD fTimer;
   BCMenu fMenu, fDefaultMenu;
};

///////////////////////////////////////////////////////////////////////////////
// inline functions
inline DWORD AMainFrame::GetTimer() const
{
   return fTimer;
}


#endif // hMainFrm
