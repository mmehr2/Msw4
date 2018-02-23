// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "ScrollDialog.h"
#include "MainFrm.h"
#include "MswView.h"
#include "RtfMemFile.h"

#include <dbt.h>
#include <math.h>
#include <set>

static const BYTE fDeltaPixelsTable[] = {
   0,  1,  1,  1,  1,  
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
   1, 70,70,40,40,
   40,25,25,25,15, 
   15,15,10,10,10, 
   8, 8, 8, 5, 5, 
   5, 4, 4, 4, 4, 
   3, 3, 3, 3, 3, 
   2, 2, 2, 2, 2, 
   2, 2, 2, 2, 2, 
   3, 3, 3, 3, 3, 
   4, 4, 4, 4, 4, 
   3, 3, 3, 3, 3, 
   2, 2, 2, 2, 2, 
   2, 2, 2, 2, 2, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1, 
   1, 1, 1, 1, 1
};


static const int kMaxSpeed             = 127;
static const int kKeyboardScrollDelta  = 2;
static const int kHalfDeadZoneWidth    = 2;
static const int kTimerHeight          = 20;

static const int kMinFrameInterval     = 1;
static const int kMaxFrameInterval     = 50;
int AScrollDialog::gMinFrameInterval   = 5;
DWORD gFrameIntervalChanged            = 0;

static UINT WM_MSW_SCROLL = ::RegisterWindowMessage(_T("MSW_SCROLL"));

#if MS_DEMO 
   static const int kDemoTextHeight = 192;
   static const int kDemoTimeout    = 20 * 1000;   // milliseconds
   static const int kDemoFlash      = 3 * 1000;    // milliseconds
   static BITMAP gDemoBitmap        = {0};
   static CDC gDemoDc;
   static void InitDemo(CDC* pDC);
#endif                              

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
   ON_REGISTERED_MESSAGE(AMswApp::sSetScrollPosMsg, &OnSetScrollPos)
   ON_REGISTERED_MESSAGE(AMswApp::sSetScrollPosSyncMsg, &OnSetScrollPosSync)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(AScrollDialog, CDialog)
AScrollDialog::AScrollDialog(CWnd* pParent /*=NULL*/) :
   CDialog(AScrollDialog::IDD, pParent),
   fCursor(NULL),
   fAccel(NULL),
   fScrollAccel(NULL),
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

   ::SetCursor(theApp.fShowSpeed ? fCursor : NULL);
   ::ShowCursor(theApp.fShowSpeed);
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

static DWORD CALLBACK MemFileOutCallBack(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   ((CFile*)dwCookie)->Write(pbBuff, cb);
   *pcb = cb;
   return 0;
}

BOOL AScrollDialog::OnInitDialog()
{
   CDialog::OnInitDialog();

   fCursor = AfxGetApp()->LoadCursor(rCurScroll);

   // load the accelerator table
   fAccel = ::LoadAccelerators(::AfxGetInstanceHandle(), MAKEINTRESOURCE(rMainFrame));
   ASSERT(NULL != fAccel);
   fScrollAccel = ::LoadAccelerators(::AfxGetInstanceHandle(), MAKEINTRESOURCE(rScrollAccelerators));
   ASSERT(NULL != fScrollAccel);

   // turn off redraw while things are being repositioned
   this->SetRedraw(FALSE);
   fRichEdit.SetRedraw(FALSE);

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

   // wrap to window
   fRichEdit.SetTargetDevice(NULL, 0);
   fRtfHelper.SetDefaultFont();

   // create the divider, if necessary
   CString divider;
   const bool dividers = (theApp.fShowDividers && 
      (theApp.fLoop || (theApp.fLink && (fScripts.size() > 1))));
   if (dividers)
   {  // draw a script divider
      CDC* dc = fRichEdit.GetDC();
      CFont font;
	   CFontDialog dlg(theApp.fDefaultFont);
      dlg.FillInLogFont(theApp.fDefaultFont);
	   VERIFY(font.CreateFontIndirect(&dlg.m_lf));
	   CFont* oldFont = dc->SelectObject(&font);

      const CSize s = dc->GetTextExtent(_T("_"));
      CRect r(theApp.fScrollMarginLeft, 0, theApp.fScrollMarginRight, 0);
      divider = CString(_T('_'), (r.Width() / s.cx) - 1) + _T("\r\n");

      dc->SelectObject(oldFont);
      fRichEdit.ReleaseDC(dc);
   }

   // show full-screen
   this->ShowWindow(SW_MAXIMIZE);

   // this really shouldn't be necessary, but Vista sets negative borders
   CRect r;
   this->GetWindowRect(r);
   this->MoveWindow(0, 0, r.Width(), r.Height(), FALSE);

   // build document with all scripts to be scrolled
   CRichEditCtrl& e = fRichEdit;
   e.LimitText(INT_MAX);   // limit to 2 GB
   fScripts = theApp.fScriptQueue.GetQueuedScripts();
   long nChars = 0;
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
      {  // draw a script divider at the end
         e.SetSel(-1, -1);
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
   fRtfHelper.ValidateMarks(true);
#endif // _DEBUG

#ifdef _REMOTE
   // make this window the target for remote commands
   gComm.SetParent(m_hWnd);
   if (gComm.IsMaster())
   {  // spit out the file to be scrolled, and send it to the remote installation
      LPCTSTR fileName = _T("MswScript.rtf");
      CFile scrollFile(fileName, CFile::modeCreate | CFile::modeWrite);
      EDITSTREAM editStream;
      editStream.dwCookie = (DWORD_PTR)&scrollFile;
      editStream.pfnCallback = MemFileOutCallBack;
      e.StreamOut(SF_RTF, editStream);
      scrollFile.Close();

      gComm.SendFile(fileName);
      gComm.SendCommand(AComm::kScroll, AComm::kOn);
   } else if (gComm.IsSlave() && !this->IsTopParentActive()) {
      this->ActivateTopParent();
   }
#endif // _REMOTE

   CRtfMemFile::SetTextColor(e, theApp.fInverseOut ? kWhite : kBlack, !theApp.fColorOut, true);

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
   this->OnSetScrollPos(scrollPos);

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

#if MS_DEMO
   const DWORD kEntryTime = prevFrameTime;
   DWORD demoMessageTime = prevFrameTime;
   CDC* dc = fRichEdit.GetDC();
   ::InitDemo(dc);
   fRichEdit.ReleaseDC(dc);
#endif

   // clip the mouse cursor
   this->ClipCursor();
   VERIFY(::SetCursorPos(fMouseRect.CenterPoint().x, fMouseRect.CenterPoint().y));
   VERIFY(::GetCursorPos(&fPausedPos));

   theApp.fCaption.BeginCaption(fRtfHelper);

#ifdef _DEBUG
   std::set<int> ignoredMessages;
#endif // _DEBUG

//static int z = 0;
   while (m_nFlags & WF_CONTINUEMODAL)
   {  // scroll!
      int pixels=0, intervals=0;
      const int newSpeed = this->GetSpeed(pixels, intervals);
#ifdef _REMOTE
      if ((fSpeed != newSpeed) && gComm.IsMaster()) {
         gComm.SendCommand(AComm::kScrollSpeed, newSpeed);
      }
#endif // _REMOTE

      theApp.fCaption.UpdateCaption(fRtfHelper);

      fSpeed = newSpeed;
      if (0 == fSpeed)
         ::Sleep(100);  // allow other tasks to have the processor

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

         // do the Windows scroll function...
         // only scroll the script portion of the screen...
         if (fSpeed > 0)
            deltaPos = -deltaPos;
         fRichEdit.Scroll(deltaPos);
#ifdef _DEBUG
const int kAcceptablePause = 5;
const DWORD elapsed = (clock.Now() - prevFrameTime);
if (fSpeed && (elapsed > (currentFrameInterval + kAcceptablePause)))
   TRACE1("SLOW!!!\t%d\n", elapsed - currentFrameInterval);
#endif // _DEBUG

         do {  // check for messages while we're killing time...
            MSG msg;
            if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {  // we've got messages in our queue
               if (!this->PreTranslateMessage(&msg) &&
                  ((WM_DEVICECHANGE == msg.message) ||
                      (WM_COMMAND == msg.message) ||
                      (AMswApp::sSetScrollSpeedMsg == msg.message) ||
                      (AMswApp::sSetScrollPosMsg == msg.message) ||
                      (AMswApp::sSetScrollPosSyncMsg == msg.message)) ||
                      (WM_USER == msg.message)) // blocking this message causes hang on shutdown
               {
                  ::DispatchMessage(&msg);
               }
#ifdef _DEBUG
               else { ignoredMessages.insert(msg.message); }
#endif // _DEBUG
            }
         } while ((clock.Now() - prevFrameTime) < currentFrameInterval);

//if (0 == (z++ % 20)) TRACE1("frame time: %d\n", clock.Now() - prevFrameTime);
         prevFrameTime = clock.Now();
      }

#if MS_DEMO
      if ((clock.Now() - kEntryTime) > kDemoTimeout)
         this->OnClose();
      if ((clock.Now() - demoMessageTime) > kDemoFlash)
      {  // display the 'DEMO' bitmap
         CRect r;
         CSize s(gDemoBitmap.bmWidth, gDemoBitmap.bmHeight);

         fRichEdit.GetClientRect(r);
         CDC* dc = fRichEdit.GetDC();
         dc->BitBlt(r.CenterPoint().x - s.cx/2, r.CenterPoint().y - s.cy/2, 
            s.cx, s.cy, &gDemoDc, 0, 0, SRCCOPY);
         fRichEdit.ReleaseDC(dc);

         demoMessageTime = clock.Now();
      }
#endif

      // update the clock every second
      if ((prevFrameTime - lastTimerUpdate) > 1000)
      {
         this->DrawTimer();
         lastTimerUpdate = prevFrameTime;

#ifdef _REMOTE
         // send out synchronization message
         if (!gComm.IsMaster()) {
            gComm.SendCommand(AComm::kScrollPosSync, fRtfHelper.GetCurrentUserPos());
         }
#endif // _REMOTE
      }

      if (theApp.fShowPos) // draw the script position
         fScrollPos.Update(fRtfHelper.GetCurrentUserPos(), *this);
   }

   return 0;
}

void AScrollDialog::SetHandControllerSpeed(int speed)
{  // put the cursor at the new speed. The speed is assumed
   // to be {-128, 128}.
   ASSERT(speed >= -128);
   ASSERT(speed <= 128);

   int newSpeed = speed;
   const int rangeY = fMouseRect.Height() / 2;
   const int y = fMouseRect.CenterPoint().y - (rangeY * newSpeed) / kMaxSpeed;
   const int mouseX = fMouseRect.CenterPoint().x;
   ::SetCursorPos(mouseX, y);
}

LRESULT AScrollDialog::OnSetScrollPos(WPARAM pos, LPARAM)
{
   fRtfHelper.SetCurrentUserPos(pos);
#ifdef _REMOTE
   if (gComm.IsMaster()) {
      gComm.SendCommand(AComm::kScrollPos, pos);
   }
#endif // _REMOTE
   fRichEdit.RedrawWindow();
   return 0;
}

LRESULT AScrollDialog::OnSetScrollPosSync(WPARAM pos, LPARAM)
{
   const int newPos = static_cast<int>(pos);
   int curPos = fRtfHelper.GetCurrentUserPos();
   const int diff = ::abs(curPos - newPos);
   if (diff > (::GetSystemMetrics(SM_CYSCREEN) / 10)) {
      fRtfHelper.SetCurrentUserPos(newPos);
      fRichEdit.RedrawWindow();
   }

   return 0;
}

void AScrollDialog::OnClose()
{
   this->OnToggleScroll();
   CDialog::OnClose();
}

BOOL AScrollDialog::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
   ::SetCursor(theApp.fShowSpeed ? fCursor : NULL);
   ::ShowCursor(theApp.fShowSpeed);

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

      return ::TranslateAccelerator(m_hWnd, fAccel, pMsg) ||
         ::TranslateAccelerator(m_hWnd, fScrollAccel, pMsg);
   } 
   return CDialog::PreTranslateMessage(pMsg);
}

void AScrollDialog::OnToggleScroll()
{
   const MSG* msg = this->GetCurrentMessage();
   if (AComm::kOn == HIWORD(msg->wParam)) { // enter scroll mode - we're already there!
      return;
   }

   fRtfHelper.fArt.Stop();
   theApp.fCaption.EndCaption();

   this->UnclipCursor();

   // set the active script and current position
   int pos = fRtfHelper.GetCurrentUserPos();
   AScript s = fScripts.GetScriptFromPos(pos);
   ASSERT(NULL != s.GetView());
   AMainFrame* frame = dynamic_cast<AMainFrame*>(::AfxGetMainWnd());
   frame->MDIActivate(s.GetView()->GetParent());
   pos -= s.GetStart();
   s.GetView()->fRtfHelper.SetCurrentUserPos(pos);

#ifdef _REMOTE
   if (gComm.IsMaster()) {
      gComm.SendCommand(AComm::kScroll, AComm::kOff);
   }
#endif // _REMOTE

   CDialog::OnCancel();
}

void AScrollDialog::OnGotoBookmark(UINT uId)
{
   if (fRtfHelper.GotoBookmark(uId)) {
      this->OnSetScrollPos(fRtfHelper.GetCurrentUserPos());
      this->Pause(false);
   }
}

void AScrollDialog::OnNextBookmark()
{
   if (fRtfHelper.NextBookmark()) {
      this->OnSetScrollPos(fRtfHelper.GetCurrentUserPos());
      this->Pause(false);
   }
}

void AScrollDialog::OnPrevBookmark()
{
   if (fRtfHelper.PrevBookmark()) {
      this->OnSetScrollPos(fRtfHelper.GetCurrentUserPos());
      this->Pause(false);
   }
}

void AScrollDialog::OnSetPaperclip()
{
   fRtfHelper.SetPaperclip();

   // save this paperclip in the original document as well
   const int pos = fRtfHelper.GetCurrentUserPos();
   AScript s = fScripts.GetScriptFromPos(pos);
   ASSERT(NULL != s.GetView());
   const int topChar = fRichEdit.CharFromPos(CPoint(0,s.GetStart()));
   s.GetView()->fRtfHelper.fPaperclips.Toggle(
      fRichEdit.CharFromPos(CPoint(0,pos)) - topChar);
   fRtfHelper.ValidateMarks(false);
}

void AScrollDialog::OnNextPaperclip()
{
   if (fRtfHelper.NextPaperclip()) {
      this->OnSetScrollPos(fRtfHelper.GetCurrentUserPos());
      this->Pause(false);
   }
}

void AScrollDialog::OnPrevPaperclip()
{
   if (fRtfHelper.PrevPaperclip()) {
      this->OnSetScrollPos(fRtfHelper.GetCurrentUserPos());
      this->Pause(false);
   }
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
         this->OnSetScrollPos(s.GetStart() + fRtfHelper.sCueMarker.GetPosition().y);
      else
         this->OnSetScrollPos(s.GetEnd() - (r.Height() - fRtfHelper.sCueMarker.GetPosition().y));

      fRichEdit.RedrawWindow();
      this->Pause(false);  // again, to reset positions
   }
   else if ((VK_LEFT == nChar) && (gMinFrameInterval < kMaxFrameInterval)) {
      ++gMinFrameInterval;
      gFrameIntervalChanged = ATimer().Now();
#ifdef _REMOTE
      if (gComm.IsMaster()) {
         gComm.SendCommand(AComm::kFrameInterval, gMinFrameInterval);
      }
#endif // _REMOTE
   } else if ((VK_RIGHT == nChar) && (gMinFrameInterval > kMinFrameInterval)) {
      --gMinFrameInterval;
      gFrameIntervalChanged = ATimer().Now();
#ifdef _REMOTE
      if (gComm.IsMaster()) {
         gComm.SendCommand(AComm::kFrameInterval, gMinFrameInterval);
      }
#endif // _REMOTE
   }

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
   const MSG* msg = this->GetCurrentMessage();
   if ((AComm::kOn == HIWORD(msg->wParam) && t.IsRunning()) ||
       (AComm::kOff == HIWORD(msg->wParam) && !t.IsRunning())) {
      return;
   }

   theApp.fTimerOn = !t.IsRunning();
   (theApp.fTimerOn) ? t.Resume() : t.Stop();

#ifdef _REMOTE
   if (gComm.IsMaster()) {
      gComm.SendCommand(AComm::kTimer, theApp.fTimerOn ? AComm::kOn : AComm::kOff);
   }
#endif // _REMOTE
}

void AScrollDialog::OnTimerReset()
{
   fRtfHelper.fArt.Reset();
   this->DrawTimer();

#ifdef _REMOTE
   if (gComm.IsMaster()) {
      gComm.SendCommand(AComm::kTimer, AComm::kReset);
   }
#endif // _REMOTE
}

static double GetSpeedConstant(const CRect& mouseRect)
{
   static double a = 0.0;
   static CRect rMouse;
   if (rMouse != mouseRect) {
      rMouse = mouseRect;
      const double y = kMaxSpeed;
      const double x = rMouse.Height() / 2.0;
      const double c = kHalfDeadZoneWidth;
      a = ((y - c) / ::pow(x, 2)) * 1.5;
   }
   return a;
}

static int CalcSpeed(const CRect& mouseRect, const CPoint& m, const double& a)
{
   const double x = mouseRect.CenterPoint().y - m.y;
   const double y = a * ::pow(x, 2) + kHalfDeadZoneWidth;
   int speed = (int)y;

   ASSERT(speed >= 0); // squaring x removed the sign
   if (speed <= kHalfDeadZoneWidth)
      speed = 0;

   if (x < 0)
      speed = -speed;

   if (theApp.fReverseSpeedControl)
      speed = -speed;
   
   if (speed > kMaxSpeed)
      speed = kMaxSpeed;
   else if (speed < -kMaxSpeed)
      speed = -kMaxSpeed;

   return speed;
}

int AScrollDialog::GetSpeed(int &deltaPixels, int &frameInterval)
{  // get the offset of the hand controller or mouse.
   // Translate that value into a speed that follows a parabolic
   // curve (greater control at lower speed).
//#ifndef _DEBUG
   if (!this->IsTopParentActive())
      return 0;
//#endif // !_DEBUG

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
         this->SetHandControllerSpeed(newSpeed);
   }

   // We'll use y = ax^2 + bx + c, where y=fSpeed, x=max mouse position, and b=0.
   // Calculate a (a = (y - c) / x^2) using the case of the maximum mouse
   // position and speed...

   // now fill in the values for our current position...
   POINT m = {0, 0};
   VERIFY(::GetCursorPos(&m));
   const double a = ::GetSpeedConstant(fMouseRect);
   int speed = ::CalcSpeed(fMouseRect, m, a);

   // return values from speedTable....
   int i = (speed < 0) ? -speed : speed;
   deltaPixels = fDeltaPixelsTable[i];
   frameInterval = fIntervalTable[i];  
#if 0 // _DEBUG
static int z = 0;
if (0 == (z % 100)) TRACE3("speed:%d pixels:%d intervals:%d\n", speed, deltaPixels, frameInterval);
#endif // _DEBUG

   return speed;
}

LRESULT AScrollDialog::OnSetScrollSpeed(WPARAM wParam, LPARAM)
{
   CPoint m;
   VERIFY(::GetCursorPos(&m));
   const int speed = (int)wParam;
   const int absSpeed = ::abs(speed);
   const double a = ::GetSpeedConstant(fMouseRect);
   const int absDelta = (absSpeed < kHalfDeadZoneWidth) ? 0 : (int)::sqrt((absSpeed - kHalfDeadZoneWidth) / a);
   m.y = fMouseRect.CenterPoint().y - (speed < 0 ? -absDelta : absDelta);

   int checkSpeed = 0;
   // it's possible that we can't get the desired speed with a whole-number
   // mouse position. In that case, punt...
   bool under = false, over = false;
   do { // calculate the speed from this point. If we've got rounding errors, adjust
      checkSpeed = ::CalcSpeed(fMouseRect, m, a);
      if (checkSpeed < speed) {under = true; --m.y;}
      else if (checkSpeed > speed) {over = true; ++m.y;}
   } while ((checkSpeed != speed) && (!under || !over));

   VERIFY(::SetCursorPos(m.x, m.y));

   // the speed is a percentage of the speed rect from the middle
   // multiplied by 100
   //CPoint m;
   //VERIFY(::GetCursorPos(&m));
   //const CPoint center = fSpeedRect.CenterPoint();
   //m.y = center.y - (int)(int(speed) / 1000.0 * fSpeedRect.Height());
   //VERIFY(::SetCursorPos(m.x, m.y));
   return 0;
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

#if 0 //_DEBUG
   // display the speed
   text.Format(_T("Speed: %d"), fSpeed);
   CRect speedRect(fTimerRect);
   speedRect.OffsetRect(0, 20);
   dc.FillSolidRect(speedRect, theApp.fInverseOut ? kBlack : kWhite);
   dc.DrawText(text, speedRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
#endif // _DEBUG

#if 1 //_DEBUG
   // display the scan interval
   const bool show = (ATimer().Now() < (gFrameIntervalChanged + 3*1000));
   CRect speedRect(fTimerRect);
   speedRect.OffsetRect(0, 40);
   dc.FillSolidRect(speedRect, theApp.fInverseOut ? kBlack : kWhite);
   if (show) {
      dc.DrawText(_T("Scan Interval:"), speedRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
      speedRect.OffsetRect(0, 20);
      dc.FillSolidRect(speedRect, theApp.fInverseOut ? kBlack : kWhite);
      text.Format(_T("%d ms"), gMinFrameInterval);
      dc.DrawText(text, speedRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
   } else {
      speedRect.OffsetRect(0, 20);
      dc.FillSolidRect(speedRect, theApp.fInverseOut ? kBlack : kWhite);
   }
#endif // _DEBUG
}

void AScrollDialog::Pause(bool toggle)
{
   if (toggle && (0 == fSpeed))
      ::SetCursorPos(fPausedPos.x, fPausedPos.y);
   else    
   {  // pause
      ::GetCursorPos(&fPausedPos);
      ::SetCursorPos(fPausedPos.x, fMouseRect.CenterPoint().y);
      theApp.fCaption.ResetCaption(fRtfHelper);
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
//#ifndef _DEBUG
   if (WA_INACTIVE == nState)
   {  // pause scrolling and unclip
      this->Pause(false);
      this->UnclipCursor();
   }
   else
   {  // set to speed=0 and clip the cursor
      VERIFY(::SetCursorPos(fMouseRect.CenterPoint().x, fMouseRect.CenterPoint().y));
      this->ClipCursor();
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
//#endif // !_DEBUG
}

#if MS_DEMO
void InitDemo(CDC* pDC)
{
   if (NULL == gDemoDc.m_hDC)
   {
      CBitmap bitmap;
      bitmap.LoadBitmap(rBmpDemo);
      gDemoDc.CreateCompatibleDC(pDC);
      gDemoDc.SelectObject(&bitmap);
      bitmap.GetObject(sizeof(BITMAP), &gDemoBitmap);
   }
}
#endif
