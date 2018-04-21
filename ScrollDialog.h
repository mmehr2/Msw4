// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

//-----------------------------------------------------------------------------
// includes
#pragma once

#include "Msw.h"
#include "RichEditScrollCtrl.h"
#include "RtfHelper.h"
#include "ScrollPosMarker.h"


//-----------------------------------------------------------------------------
// classes
class AMainFrame;

class AScrollDialog : public CDialog
{
public:
   enum {IDD = rDlgScroll};

   AScrollDialog(CWnd* pParent = NULL);   // standard constructor

   void RestorePosition();

private: // methods
   DECLARE_DYNAMIC(AScrollDialog)
   void ClipCursor();
   void UnclipCursor();
   void ResizeChildControls();
   void SetHandControllerSpeed(int speed);
   LRESULT OnScroll(WPARAM, LPARAM);
   LRESULT OnSetScrollSpeed(WPARAM speed, LPARAM=0);
   LRESULT OnSetScrollPos(WPARAM pos, LPARAM=0);
   LRESULT OnSetScrollPosSync(WPARAM pos, LPARAM=0);

   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   virtual BOOL PreTranslateMessage(MSG* pMsg);

   DECLARE_MESSAGE_MAP()

   afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
   afx_msg void OnClose();
   afx_msg void OnDestroy();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
   afx_msg void OnTogglePaperclip();
   afx_msg void OnToggleScroll();
   afx_msg void OnGotoBookmark(UINT uId);
   afx_msg void OnNextBookmark();
   afx_msg void OnPrevBookmark();
   afx_msg void OnNextPaperclip();
   afx_msg void OnPrevPaperclip();
   afx_msg void OnSetPaperclip();
   afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg void OnPaint();
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
   afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
   afx_msg void OnToggleTimer();
   afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg void OnTimerReset();
   afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
#ifdef _REMOTE
	afx_msg LRESULT OnUpdateData(WPARAM wParam, LPARAM lParam);
#endif//def _REMOTE

   void HandleButton(int action);
   int GetSpeed(int &deltaPixels, int &frameInterval);
   void DrawTimer();
   void Pause(bool toggle);
   afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

public:  // data
   ARtfHelper fRtfHelper;
   static int gMinFrameInterval;

private: // data
   ARichEditScrollCtrl fRichEdit;
   CRect fMouseRect;
   CRect fSpeedRect;
   HCURSOR fCursor;
   HACCEL fAccel;
   HACCEL fScrollAccel;
   AScriptQueue fScripts;
   int fSpeed;
   CRect fTimerRect;
   CPoint fPausedPos;
   bool fClearPaperclips;
   AScrollPosIndicator fScrollPos;
};

//-----------------------------------------------------------------------------
// inline functions
