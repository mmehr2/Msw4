// Copyright � 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "ScrollDialog.h"
#include "MainFrm.h"
#include "MswView.h"

#include <dbt.h>

static const BYTE fDeltaPixelsTable[] = {
   1,  1,  1,  1,  1,  
   1,  1,  1,  1,  1,  
   1,  1,  1,  1,  1,  
   1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  
   1,  1,  1,  1,  1,  
   1,  1,  1,  1,  1,  
   1,  1,  1,  1,  1,
   2,  2,  2,  2,  2,  
   3,  3,  3,  3,  3,  
   3,  3,  3,  3,  3,  
   3,  3,  3,  3,  3,
   3,  3,  3,  3,  3,  
   2,  2,  2,  2,  2,  
   3,  3,  3,  3,  3,  
   3,  3,  3,  3,  3,
   3,  3,  3,  3,  3,  
   4,  4,  4,  4,  4,  
   5,  5,  5,  5,  5,  
   6,  6,  6,  6,  6,
   6,  6,  6,  6,  6,  
   10, 10, 10, 10, 10, 
   13, 13, 13, 13, 13, 
   16, 16, 16, 16, 16,
   20, 20, 20, 20, 20, 
   20, 20, 20, 20, 20
};

const static BYTE fIntervalTable[] = {
   70, 70, 70, 40, 40,
   40, 25, 25, 25, 15, 
   15, 15, 10, 10, 10, 
   8,  8,  8,  5,  5, 
   5,  4,  4,  4,  4, 
   3,  3,  3,  3,  3, 
   2,  2,  2,  2,  2, 
   2,  2,  2,  2,  2, 
   3,  3,  3,  3,  3, 
   4,  4,  4,  4,  4, 
   3,  3,  3,  3,  3, 
   2,  2,  2,  2,  2, 
   2,  2,  2,  2,  2, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1,
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1, 
   1,  1,  1,  1,  1
};

static const int kMaxSpeed             = 127;
static const int kKeyboardScrollDelta  = 2;
static const int kHalfDeadZoneWidth    = 4;
static const int kTimerHeight          = 20;

static const int kMinFrameInterval     = 1;
static const int kMaxFrameInterval     = 50;
static int gMinFrameInterval           = kMinFrameInterval;
//static int gMaxIntervals[kMaxSpeed]    = {0};

static UINT WM_MSW_SCROLL = ::RegisterWindowMessage(_T("MSW_SCROLL"));

BEGIN_MESSAGE_MAP(AScrollDialog, CDialog)
   ON_WM_ERASEBKGND()
   ON_WM_PARENTNOTIFY()
   ON_WM_CLOSE()
   ON_WM_SETCURSOR()
   ON_WM_DESTROY()
   ON_WM_WINDOWPOSCHANGING()
   ON_WM_SIZE()
   ON_WM_SHOWWINDOW()
   ON_COMMAND(rCmdToggleScrollMode, &OnToggleScroll)
   ON_COMMAND_RANGE(rCmdGotoBkMk1, rCmdGotoBkMk1 + ABookmark::kMaxCount, &OnGotoBookmark)
   ON_COMMAND(rCmdNextBkmk, &OnNextBookmark)
   ON_COMMAND(rCmdPrevBkmk, &OnPrevBookmark)
   ON_COMMAND(rCmdTogglePaperclip, &OnSetPaperclip)
   ON_COMMAND(rCmdNextPclip, &OnNextPaperclip)
   ON_COMMAND(rCmdPrevPclip, &OnPrevPaperclip)
   ON_WM_PAINT()
   ON_WM_LBUTTONUP()
   ON_WM_RBUTTONUP()
   ON_COMMAND(rCmdToggleTimer, &OnToggleTimer)
   ON_COMMAND(rCmdTimerReset, &OnTimerReset)
   ON_REGISTERED_MESSAGE(WM_MSW_SCROLL, OnScroll)
   ON_WM_DEVICECHANGE()
   ON_WM_ACTIVATE()
   ON_REGISTERED_MESSAGE(AMswApp::sSetScrollSpeedMsg, &OnSetScrollSpeed)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(AScrollDialog, CDialog)
AScrollDialog::AScrollDialog(CWnd* pParent /*=NULL*/) :
   CDialog(AScrollDialog::IDD, pParent),
   fCursor(NULL),
   fAccel(NULL),
   fRtfHelper(true),
   fClearPaperclips(false)
{
   fRtfHelper.fParent = this;
   fRtfHelper.fEdit = &fRichEdit;
}

void AScrollDialog::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, rCtlRichEdit, fRichEdit);
}

void AScrollDialog::ClipCursor()
{  // shrink the clip area down to a vertical line
   VERIFY(::ClipCursor(fMouseRect));
   this->SetCapture();

   if (theApp.fShowSpeed)
      ::SetCursor(fCursor);
   else
      ::ShowCursor(FALSE);
}

void AScrollDialog::UnclipCursor()
{
   ::ShowCursor(TRUE);
   VERIFY(::ClipCursor(NULL));
   VERIFY(::ReleaseCapture());
}

void AScrollDialog::ResizeChildControls()
{
   ASSERT(NULL != fRichEdit.GetSafeHwnd());

   CRect r;
   fRichEdit.GetWindowRect(r);
   this->ScreenToClient(r);

   // compute the rectangle for the timer
   fTimerRect.SetRect(0, 0, r.left, kTimerHeight);

   // Set Mouse Clipping Rect
   fSpeedRect.left = 0;
	fSpeedRect.right = r.left / 3;
	fSpeedRect.top = r.top + (r.Height() / 4);
	fSpeedRect.bottom = r.Height() - (r.Height() / 4);
   fMouseRect = fSpeedRect;

   // now setup the clipping rect in screen coords
   ClientToScreen(fMouseRect);
   if (fMouseRect.left < 0)
      fMouseRect.left = 0;
   fMouseRect.right = fMouseRect.left + 1;
}

BOOL AScrollDialog::OnInitDialog()
{
   CDialog::OnInitDialog();

   fCursor = AfxGetApp()->LoadCursor(rCurScroll);

   // load the accelerator table
   fAccel = ::LoadAccelerators(::AfxGetInstanceHandle(), MAKEINTRESOURCE(rMainFrame));
   ASSERT(NULL != fAccel);

   // make this window the target for remote commands
   mDebugOnly(gComm.SetParent(m_hWnd));

   // get the cue marker and scroll position. We've got to do this before 
   // we start messing with copying the scripts. Otherwise, the scroll 
   // position might change.
   const size_t s = theApp.fScriptQueue.GetActive();
   ASSERT(s < theApp.fScriptQueue.size());
   AMswView* activeView = theApp.fScriptQueue[s].GetView();
   // set the cue marker and scroll positions
   fRtfHelper.sCueMarker.SetPosition(-1, 
      activeView->fRtfHelper.sCueMarker.GetPosition().y);
   int scrollPos = activeView->fRtfHelper.GetCurrentUserPos();

   fClearPaperclips = (theApp.fClearPaperclips || 
      (0x8000 & ::GetAsyncKeyState(VK_MENU)));

   // turn off redraw while things are being repositioned
   this->SetRedraw(FALSE);
   fRichEdit.SetRedraw(FALSE);

   // wrap to window
   fRichEdit.SetTargetDevice(NULL, 0);

   // show full-screen
   this->ShowWindow(SW_MAXIMIZE);

   // this really shouldn't be necessary, but Vista sets negative borders
   CRect r;
   this->GetWindowRect(r);
   this->MoveWindow(0, 0, r.Width(), r.Height(), FALSE);

   // build document with all scripts to be scrolled
   CDC* dc = NULL;
   CString divider;
   CRichEditCtrl& e = fRichEdit;
   e.LimitText(INT_MAX);   // limit to 2 GB
   fScripts = theApp.fScriptQueue.GetQueuedScripts();
   long nChars = 0;
   const bool dividers = (theApp.fShowDividers && 
      (theApp.fLoop || (theApp.fLink && (fScripts.size() > 1))));
   for (size_t i = 0; i < fScripts.size(); ++i)
   {
      AScript& s = fScripts[i];
      ASSERT(s.GetScroll());

      CHARRANGE sel;
      {  // copy the text ouf of this view
         CRichEditCtrl& e2 = s.GetView()->GetRichEditCtrl();
         e2.SetRedraw(FALSE);
         e2.GetSel(sel);
         e2.SetSel(0, -1);       // select all text
         e2.Copy();              // copy to clipboard
         e2.SetSel(sel);
         e2.SetRedraw(TRUE);
      }

      long start=0, end=0;
      e.SetSel(-1, -1);          // go to end
      e.GetSel(start, end);
      s.SetStart(e.PosFromChar(end).y - e.PosFromChar(0).y);
      ASSERT(e.CanPaste());
      e.Paste();                 // paste in the e2 text

      e.SetSel(-1, -1);        // go to end
      e.ReplaceSel(_T("\r\n"));  // add a blank line between the scripts

      // transfer the paperclips and bookmarks
      APaperclips& paperclips = s.GetView()->fRtfHelper.fPaperclips;
      if (fClearPaperclips)
         paperclips.clear();
      else for (size_t j = 0; j < paperclips.size(); ++j)
      {
         AMarker paperclip(paperclips[j]);
         paperclip.fPos += nChars;
         fRtfHelper.fPaperclips.push_back(paperclip);
      }

      ABookmarks& bookmarks = s.GetView()->fRtfHelper.fBookmarks;
      for (size_t j = 0; j < bookmarks.size(); ++j)
      {
         ABookmark bookmark(bookmarks[j]);
         bookmark.fPos += nChars;
         fRtfHelper.fBookmarks.push_back(bookmark);
      }

      if (dividers)
      {  // draw a script divider
         if (NULL == dc)
         {  // just create the divider once
            dc = e.GetDC();
            CSize s = dc->GetTextExtent(_T("_"));
            CRect r(theApp.fScrollMarginLeft, 0, theApp.fScrollMarginRight, 0);
            divider = CString(_T('_'), r.Width() / s.cx) + _T("\r\n");
         }

         e.SetSel(-1, -1);          // go to end
         e.ReplaceSel(divider);
      }

      // now record the end position for this script
      e.SetSel(-1, -1);          // go to end
      e.GetSel(start, end);
      s.SetEnd(e.PosFromChar(end).y - e.PosFromChar(0).y);
      nChars = end;
   }
#ifdef _DEBUG
   // dump the composite file, bookmarks, and paperclips
   fScripts.Dump();
   CFile debugFile(_T("msw file to scroll.rtf"), CFile::modeCreate | CFile::modeWrite);
   Utils::StreamToFile(e, SF_RTF, debugFile);
   fRtfHelper.ValidateMarks(true);
#endif // _DEBUG

   Utils::SetTextColor(e, theApp.fInverseOut ? kWhite : kBlack, !theApp.fColorOut);

   // restart the clock
   if (theApp.fTimerOn)
      fRtfHelper.fArt.Resume();

   // disable the edit control to hide the caret
   const long style = ::GetWindowLong(fRichEdit.m_hWnd, GWL_STYLE);
   ::SetWindowLong(fRichEdit.m_hWnd, GWL_STYLE, style | WS_DISABLED); 
   fRichEdit.SetSel(0, 0);

   // set the scroll position
   for (size_t i = 0; i < fScripts.size(); ++i)
      if (fScripts[i].GetView() == activeView)
         scrollPos += fScripts[i].GetStart();
   fRtfHelper.SetCurrentUserPos(scrollPos);

   this->PostMessage(WM_MSW_SCROLL);

   return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL AScrollDialog::OnEraseBkgnd(CDC* pDC)
{
   pDC->SaveDC();

   CRect rcClient;
   this->GetClientRect(rcClient);
   pDC->FillSolidRect(rcClient, theApp.fInverseOut ? kBlack : kWhite);

   if (theApp.fShowSpeed)
   {  // draw the mouse clip rect
      CRect r = fSpeedRect;
      const COLOR16 c1 = 0xa000;
      const COLOR16 c2 = 0x0a00;
      TRIVERTEX vert[4] = {{r.left, r.top, c1, c1, c1, 0},
                           {r.right, r.CenterPoint().y, c2, c2, c2, 0},
                           {r.left, r.CenterPoint().y, c2, c2, c2, 0},
                           {r.right, r.bottom, c1, c1, c1, 0}};
      GRADIENT_RECT grad[1] = {0, 1};
      ::GradientFill(pDC->m_hDC, vert, 2, grad, _countof(grad), GRADIENT_FILL_RECT_V);
      ::GradientFill(pDC->m_hDC, &vert[2], 2, grad, _countof(grad), GRADIENT_FILL_RECT_V);

      // draw the center line
      r.bottom = r.CenterPoint().y;
      ::DrawEdge(pDC->m_hDC, r, BDR_SUNKENINNER, BF_BOTTOM);
   }

   pDC->RestoreDC(-1);
   return TRUE;
}

LRESULT AScrollDialog::OnScroll(WPARAM, LPARAM)
{
   CPoint p(0,0);
   fRichEdit.SendMessage(EM_SETSCROLLPOS, 0, (LPARAM)&p);

   // restore redraw
   this->SetRedraw(TRUE);
   fRichEdit.SetRedraw(TRUE);

   // make sure all dimensions are updated
   this->ResizeChildControls();

   // do initial draw
   fRichEdit.Initialize();
   VERIFY(fRichEdit.RedrawWindow());
   VERIFY(this->RedrawWindow());
   this->InvalidateRect(fRtfHelper.GetCueBarRect());
   this->DrawTimer();
   if (theApp.fShowPos)
   {
      CRect r(fSpeedRect);
      r.OffsetRect(r.Width(), 0);
      r.right -= r.Width() / 2;
      CClientDC dc(this);
      fScrollPos.Create(r, dc, fScripts, fRichEdit.GetHeight());
   }

   ATimer clock;
   DWORD prevFrameTime = clock.Now();
   DWORD lastTimerUpdate = prevFrameTime;

   // clip the mouse cursor
   this->ClipCursor();
   VERIFY(::SetCursorPos(fMouseRect.CenterPoint().x, fMouseRect.CenterPoint().y));
   VERIFY(::GetCursorPos(&fPausedPos));

   while (m_nFlags & WF_CONTINUEMODAL)
   {  // scroll!
      int pixels=0, intervals=0;
      fSpeed = this->GetSpeed(pixels, intervals);
//      const int kMinInterval = gMaxIntervals[::abs(fSpeed)];
      if ((fSpeed > -kHalfDeadZoneWidth) && (fSpeed < kHalfDeadZoneWidth))
      {  // stop that scrolling...
         pixels = 0;
         intervals = 1;
      }
      ::Sleep(1);  // allow other tasks to have the processor

      while (intervals > 0)
      {  
         int deltaPos = (1 == intervals) ? pixels : 1;
         DWORD currentFrameInterval = gMinFrameInterval;
         if (1 == pixels)
         {
            currentFrameInterval *= intervals;
            intervals = 1;
         }
         --intervals;
         --pixels;

         do
         {  // check for messages while we're killing time...
            MSG msg;
            if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
               if (!this->PreTranslateMessage(&msg))
                  if ((WM_DEVICECHANGE == msg.message) ||
                      (WM_COMMAND == msg.message) ||
                      (AMswApp::sSetScrollSpeedMsg == msg.message))
                     ::DispatchMessage(&msg);
         } while ((clock.Now() - prevFrameTime) < currentFrameInterval);

         // do the Windows scroll function...
         // only scroll the script portion of the screen...
         if (fSpeed > 0)
            deltaPos = -deltaPos;
         fRichEdit.Scroll(deltaPos);
/*
static int x = 0;
if (0 == (x++ % 9))
   TRACE1("frame time: %d\n", clock.Now() - prevFrameTime);
*/
/*
DWORD now = clock.Now();
DWORD interval = (now - prevFrameTime);
if (interval > kMinInterval)
   gMaxIntervals[::abs(fSpeed)] = interval;
else while (interval < kMinInterval)
{
   now = clock.Now();
   interval = (now - prevFrameTime);
   ::Sleep(1);
}
*/

         prevFrameTime = clock.Now();
      }

      // update the clock every second
      if ((prevFrameTime - lastTimerUpdate) > 1000)
      {
         this->DrawTimer();
         lastTimerUpdate = prevFrameTime;
      }

      if (theApp.fShowPos) // draw the script position
         fScrollPos.Update(fRtfHelper.GetCurrentUserPos(), *this);
   }

   fRichEdit.Uninitialize();

   return 0;
}

LRESULT AScrollDialog::OnSetScrollSpeed(WPARAM speed, LPARAM)
{  // put the cursor at the new speed
   int newSpeed = speed;
   if (theApp.fReverseSpeedControl)
      newSpeed = -newSpeed;
   const int centerY = fMouseRect.CenterPoint().y;
   const int rangeY = fMouseRect.Height() / 2;
   const int y = centerY + (rangeY * newSpeed) / kMaxSpeed;
   const int mouseX = fMouseRect.CenterPoint().x;
   ::SetCursorPos(mouseX, y);
   return 0;
}

void AScrollDialog::OnClose()
{
   this->OnToggleScroll();
   CDialog::OnClose();
}

BOOL AScrollDialog::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
   if (theApp.fShowSpeed)
      ::SetCursor(fCursor);
   else
      ::ShowCursor(FALSE);

   return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

void AScrollDialog::OnDestroy()
{
   this->GetWindowPlacement(&theApp.m_scrollWindowPlacement);
   CDialog::OnDestroy();
}

void AScrollDialog::RestorePosition()
{
   WINDOWPLACEMENT empty;
   ::memset(&empty, 0, sizeof(empty));
   if (0 != ::memcmp(&empty, &theApp.m_scrollWindowPlacement, sizeof(empty)))
      this->SetWindowPlacement(&theApp.m_scrollWindowPlacement);
}

void AScrollDialog::OnWindowPosChanging(WINDOWPOS* pPos)
{
   if (!(SWP_NOSIZE & pPos->flags))
   {
      const bool zoom = !this->IsZoomed();
      this->ModifyStyle(zoom ? 0 : WS_THICKFRAME, zoom ? WS_THICKFRAME : 0);
      this->ModifyStyle(zoom ? 0 : WS_CAPTION, zoom ? WS_CAPTION : 0);
   }
   CDialog::OnWindowPosChanging(pPos);
}

void AScrollDialog::OnSize(UINT nType, int cx, int cy)
{
   CDialog::OnSize(nType, cx, cy);
   if (::IsWindow(fRichEdit.m_hWnd))
   {  // resize the edit control, but use the current left,top
      CRect r(fRtfHelper.kCueBarWidth * 2, 0, cx, cy);
      fRichEdit.MoveWindow(r, FALSE);

      fRtfHelper.Size(r.left, r.top, r.Width(), r.Height());

      this->ResizeChildControls();
   }
}

void AScrollDialog::OnShowWindow(BOOL bShow, UINT nStatus)
{
   CDialog::OnShowWindow(bShow, nStatus);
   if (bShow && ::IsWindow(fRichEdit.m_hWnd))
      this->ResizeChildControls();
}

BOOL AScrollDialog::PreTranslateMessage(MSG* pMsg)
{
   if ((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST))
   {  // check for keydown and keyup messages
      if (WM_KEYDOWN == pMsg->message)
         this->OnKeyDown(pMsg->wParam, LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
      else if (WM_KEYUP == pMsg->message)
         this->OnKeyUp(pMsg->wParam, LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
      ASSERT(NULL != fAccel);
      return ::TranslateAccelerator(m_hWnd, fAccel, pMsg);
   } 
   return CDialog::PreTranslateMessage(pMsg);
}

void AScrollDialog::OnToggleScroll()
{
   fRtfHelper.fArt.Stop();

   this->UnclipCursor();

   // set the active script and current position
   int pos = fRtfHelper.GetCurrentUserPos();
   AScript s = fScripts.GetScriptFromPos(pos);
   ASSERT(NULL != s.GetView());
   AMainFrame* frame = dynamic_cast<AMainFrame*>(::AfxGetMainWnd());
   frame->MDIActivate(s.GetView()->GetParent());
   pos -= s.GetStart();
   s.GetView()->fRtfHelper.SetCurrentUserPos(pos);

   CDialog::OnCancel();
}

void AScrollDialog::OnGotoBookmark(UINT uId)
{
   if (fRtfHelper.GotoBookmark(uId))
      this->Pause(false);
}

void AScrollDialog::OnNextBookmark()
{
   if (fRtfHelper.NextBookmark())
      this->Pause(false);
}

void AScrollDialog::OnPrevBookmark()
{
   if (fRtfHelper.PrevBookmark())
      this->Pause(false);
}

void AScrollDialog::OnSetPaperclip()
{
   fRtfHelper.SetPaperclip();

   if (!fClearPaperclips)
   {  // save this paperclip in the original document as well
      const int pos = fRtfHelper.GetCurrentUserPos();
      AScript s = fScripts.GetScriptFromPos(pos);
      ASSERT(NULL != s.GetView());
      const int topChar = fRichEdit.CharFromPos(CPoint(0,s.GetStart()));
      s.GetView()->fRtfHelper.fPaperclips.Toggle(
         fRichEdit.CharFromPos(CPoint(0,pos)) - topChar);
      fRtfHelper.ValidateMarks(false);
   }
}

void AScrollDialog::OnNextPaperclip()
{
   if (fRtfHelper.NextPaperclip())
      this->Pause(false);
}

void AScrollDialog::OnPrevPaperclip()
{
   if (fRtfHelper.PrevPaperclip())
      this->Pause(false);
}

void AScrollDialog::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{  // handle scroll-specific hot keys
   if (VK_RETURN == nChar)
      this->Pause(true);
   else if ((VK_UP == nChar) || (VK_DOWN == nChar))
   {  // simulate a mouse move
      INPUT ev;
      ::ZeroMemory(&ev, sizeof(ev));
      ev.type = INPUT_MOUSE;
      ev.mi.dwFlags = MOUSEEVENTF_MOVE;
      ev.mi.dy = (VK_UP == nChar) ? -kKeyboardScrollDelta : kKeyboardScrollDelta;
      ::SendInput(1, &ev, sizeof(ev)); 
   }
   else if ((VK_HOME == nChar) || (VK_END == nChar))
   {  // on home, go to the beginning of the current script
      // on end, go to the beginning of the last page
      this->Pause(false);

      CRect r;
      fRichEdit.GetRect(r);
      const int pos = fRtfHelper.GetCurrentUserPos();
      AScript s = fScripts.GetScriptFromPos(pos);
      if (((s.GetEnd() - s.GetStart()) < r.Height()) || (VK_HOME == nChar))
         fRtfHelper.SetCurrentUserPos(s.GetStart() + 
            fRtfHelper.sCueMarker.GetPosition().y);
      else
         fRtfHelper.SetCurrentUserPos(s.GetEnd() - 
            (r.Height() - fRtfHelper.sCueMarker.GetPosition().y));

      fRichEdit.RedrawWindow();
   }
   else if ((VK_LEFT == nChar) && (gMinFrameInterval < kMaxFrameInterval))
      ++gMinFrameInterval;
   else if ((VK_RIGHT == nChar) && (gMinFrameInterval > kMinFrameInterval))
      --gMinFrameInterval;

   CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void AScrollDialog::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{  // handle those accellerators that are not shared with the edit window
   // (these are not in the accellerator table.
   switch (nChar)
   {
      case VK_SPACE:
      if (!(0x8000 & ::GetAsyncKeyState(VK_CONTROL)))
         this->OnSetPaperclip();
      break;

      default:
      CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
      break;
   }
}

void AScrollDialog::OnPaint()
{
   fRtfHelper.Paint();
   CDialog::OnPaint();
}

void AScrollDialog::HandleButton(int action)
{
   switch (action)
   {
      case AMswApp::kScroll:        this->OnToggleScroll(); break;
      case AMswApp::kNextBookmark:  this->OnNextBookmark(); break;
      case AMswApp::kPrevBookmark:  this->OnPrevBookmark(); break;
      case AMswApp::kSetPaperclip:  this->OnSetPaperclip(); break;
      case AMswApp::kGotoPaperclip: this->OnNextPaperclip(); break;
      case AMswApp::kNone:          break;
      default:                      ASSERT(false); break;
   }
}

void AScrollDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
   this->HandleButton(theApp.fLButtonAction);
   CDialog::OnLButtonDown(nFlags, point);
}

void AScrollDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
   this->HandleButton(theApp.fRButtonAction);
   CDialog::OnRButtonUp(nFlags, point);
}

void AScrollDialog::OnToggleTimer()
{
   ATimer& t = fRtfHelper.fArt;
   theApp.fTimerOn = !t.IsRunning();
   (theApp.fTimerOn) ? t.Resume() : t.Stop();
}

void AScrollDialog::OnTimerReset()
{
   fRtfHelper.fArt.Reset();
   this->DrawTimer();
}

int AScrollDialog::GetSpeed(int &deltaPixels, int &frameInterval)
{
#ifndef _DEBUG
   if (!this->IsTopParentActive())
      return 0;
#endif // !_DEBUG

   int newSpeed = 0;
   bool lClick = false, rClick = false;
   AHandController& hc = theApp.fHandController;
   if (hc.Poll(newSpeed, lClick, rClick))
   {  // a hand controller is attached - use it's data
      if (lClick)
         this->HandleButton(theApp.fLButtonAction);
      if (rClick)
         this->HandleButton(theApp.fRButtonAction);

      if (fSpeed != newSpeed)
         this->OnSetScrollSpeed(newSpeed, 0);
   }

   // get speed from cursor position...
   POINT mouse = {0, 0};
   VERIFY(::GetCursorPos(&mouse));
   VERIFY(::SetCursorPos(mouse.x, mouse.y));
   const int centerY = fMouseRect.CenterPoint().y;
   const int rangeY = fMouseRect.Height() / 2;
   fSpeed = (kMaxSpeed * (centerY - mouse.y)) / rangeY;
   if (theApp.fReverseSpeedControl)
      fSpeed = -fSpeed;
   
   if (fSpeed > kMaxSpeed)
      fSpeed = kMaxSpeed;
   else if (fSpeed < -kMaxSpeed)
      fSpeed = -kMaxSpeed;

   // return values from speedTable....
   int i = (fSpeed < 0) ? -fSpeed : fSpeed;
   deltaPixels = fDeltaPixelsTable[i];
   frameInterval = fIntervalTable[i];  

   return fSpeed;
}

void AScrollDialog::DrawTimer()
{
   if (!theApp.fShowTimer)
      return;

   CString text;
   const double elapsed = fRtfHelper.fArt.Elapsed() / 1000;
   const int minutes = static_cast<int>(elapsed / 60.0);
   const int seconds = static_cast<int>(elapsed - (minutes * 60));
   text.Format(_T("%02d:%02d"), minutes, seconds);

   CClientDC dc(this);
   dc.SetTextColor(theApp.fInverseOut ? kWhite : kBlack);
   dc.SetBkColor(theApp.fInverseOut ? kBlack : kWhite);

   dc.FillSolidRect(fTimerRect, theApp.fInverseOut ? kBlack : kWhite);
   dc.SelectObject(::GetStockObject(DEFAULT_GUI_FONT));
   dc.DrawText(text, fTimerRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void AScrollDialog::Pause(bool toggle)
{
   if (toggle && (0 == fSpeed))
      ::SetCursorPos(fPausedPos.x, fPausedPos.y);
   else    
   {  // pause
      ::GetCursorPos(&fPausedPos);
      ::SetCursorPos(fPausedPos.x, fMouseRect.CenterPoint().y);
   }
}

BOOL AScrollDialog::OnDeviceChange(UINT nEventType, DWORD_PTR)
{
   if (DBT_DEVNODES_CHANGED == nEventType)
      theApp.fHandController.Connect();

   return TRUE;
}

void AScrollDialog::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
   CDialog::OnActivate(nState, pWndOther, bMinimized);
#ifndef _DEBUG
   if (WA_INACTIVE == nState)
   {  // pause scrolling and unclip
      this->Pause(false);
      this->UnclipCursor();
   }
   else
   {  // set to speed=0 and clip the cursor
      this->ClipCursor();
      VERIFY(::SetCursorPos(fMouseRect.CenterPoint().x, fMouseRect.CenterPoint().y));
      fRtfHelper.Paint();
      this->RedrawWindow();
      this->DrawTimer();
      if (theApp.fShowPos)
      {  // draw the script position
         CClientDC dc(this);
         fScrollPos.Draw(dc, fScripts);
         fScrollPos.Update(-1, *this);
      }
   }
#endif // !_DEBUG
}
