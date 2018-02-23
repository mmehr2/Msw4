// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"

#include "Msw.h"
#include "RtfHelper.h"
#include "BookmarkNameDialog.h"
#include "RichEditScrollCtrl.h"
#include "strings.h"

AGuiMarker ARtfHelper::sCueMarker(0, CSize(32,32));


void AGuiMarker::Draw(CDC& dc, int verticalOffset)
{
   if (NULL == fIcon)
      fIcon = ::LoadIcon(::AfxGetInstanceHandle(), MAKEINTRESOURCE(fResId));
   ::DrawIconEx(dc, fPosition.x, verticalOffset + fPosition.y, fIcon, 
      fSize.cx, fSize.cy, 0, NULL, DI_NORMAL);
}

void AGuiMarker::SetPosition(int x, int y)
{
   if (x >= 0)
      fPosition.x = x;
   if (y >= 0)
      fPosition.y = y;
}

CRect AGuiMarker::GetRect(int verticalOffset) const
{
   CRect r(fPosition, fSize);
   r.OffsetRect(0, verticalOffset);
   return r;
}


ARtfHelper::ARtfHelper(bool scrolling) :
   fParent(NULL),
   fEdit(NULL),
   fScrolling(scrolling),
   fDragging(false),
   fDragDelta(0),
   fPaperclip(rIcoPaperclip, CSize(32,32)),
   fBookmark(rIcoBookmark, CSize(32,32))
{
}

int ARtfHelper::GetCurrentUserPos()
{  // return the cue pointer position of the queue
   int pos = 0;
   if (fScrolling)
   {
      ARichEditScrollCtrl* edit = dynamic_cast<ARichEditScrollCtrl*>(fEdit);
      pos = edit->GetScrollOffset() + sCueMarker.GetPosition().y;
      const int h = edit->GetHeight();
      if (0 == h)
         pos = 0;
      else if (pos < 0)
         pos = (theApp.fLoop) ? (pos %= h) : 0;
      else if (pos > h)
         pos = (theApp.fLoop) ? (pos %= h) : h;
   }
   else
   {
      // CRichEditView limits itself to USHORT_MAX display
      // lines, so if we're past that, we need to use a 
      // rougher estimate of our location
      const int topLine = fEdit->GetFirstVisibleLine();
      const int lineIndex = fEdit->LineIndex(topLine);

      // PosFromChar returns a point relative to the top of the 
      // visible window, so we've got to scroll to the top.
      CPoint p(0,0);
      fEdit->SendMessage(EM_SETSCROLLPOS, NULL, (LPARAM)&p);
      pos = fEdit->PosFromChar(lineIndex).y - fEdit->PosFromChar(0).y +
         sCueMarker.GetPosition().y;
   }

   return pos;
}

CPoint ARtfHelper::GetPosFromChar(int chPos)
{
   return fEdit->PosFromChar(chPos);
}

void ARtfHelper::SetCurrentUserPos(int pos)
{
   pos -= sCueMarker.GetPosition().y;
   if (fScrolling)
   {
      ARichEditScrollCtrl* edit = dynamic_cast<ARichEditScrollCtrl*>(fEdit);
      edit->SetScrollOffset((pos < 0) ? 0 : pos);
   }
   else
   {
      // put the cursor in the general neighborhood
      // CharFromPos assumes the point is in client window coords,
      // so we've got to scroll the window so that we're at the top.
      CPoint p(0,0);
      fEdit->SendMessage(EM_SETSCROLLPOS, NULL, (LPARAM)&p);
      p.y = pos;
      const long chPos = fEdit->CharFromPos(p);
      fEdit->SetSel(chPos, chPos);

      // now set the scroll position for exactness
      fEdit->SendMessage(EM_SETSCROLLPOS, NULL, (LPARAM)&p);
   }

   //TRACE1("SetCurrentUserPos:\t%d\n", pos);
}

int ARtfHelper::GetCurrentUserCharPos(bool max)
{
   int charPos = 0;
   if (fScrolling)
   {  // return the character position of the cue marker
      int pos = this->GetCurrentUserPos();
      charPos = fEdit->CharFromPos(CPoint(max ? INT_MAX : 0, pos));
   }
   else
   {  // return the 0 position of the current line
      CPoint pos = fEdit->GetCaretPos();
      pos.x = (max ? INT_MAX : 0);
      charPos = fEdit->CharFromPos(pos);
   }
   return charPos;
}

void ARtfHelper::SetCurrentUserCharPos(int chPos)
{
   if (fScrolling)
   {
      CPoint p(0, fEdit->PosFromChar(chPos).y);
      this->SetCurrentUserPos(p.y);
      fEdit->RedrawWindow();
   }
   else
   {
      CPoint p(0, fEdit->PosFromChar(chPos).y);
      p.y -= fEdit->PosFromChar(0).y;
      p.y -= sCueMarker.GetPosition().y;
      fEdit->SendMessage(EM_SETSCROLLPOS, NULL, (LPARAM)&p);
      fEdit->SetSel(chPos, chPos);
      fParent->InvalidateRect(this->GetCueBarRect());
   }
}

unsigned short ARtfHelper::GetCharAt(int charPos, int& csize)
{
   ASSERT(NULL != fEdit);
   CString c;
   fEdit->GetTextRange(charPos, charPos + 1, c);
   if (c.IsEmpty()) {
      csize = 0;
      return 0;
   } else {
      csize = 1;
      return c.GetAt(0);
   }
}

long ARtfHelper::GetTextLen() const {
   ASSERT(NULL != fEdit);
   return fEdit->GetTextLength();
}

bool ARtfHelper::SetBookmark()
{
   ASSERT(!fScrolling);
   if (fBookmarks.size() < ABookmark::kMaxCount)
   {  // prompt the user for a name
      CBookmarkNameDialog dlg(fBookmarks);

      // compose a default name for the bookmark. Use the selection
      // if there is one. Otherwise, use a few words, starting with the
      // word where the cursor is located.
      CString name;
      long start = 0, end = 0;
      fEdit->GetSel(start, end);
      if ((0 == fEdit->GetTextRange(start, end, name)) && 
          (0 != fEdit->GetTextRange(max(0, start - ABookmark::kMaxName), 
            start + ABookmark::kMaxName, name)))
      {  // no selection - scan the text surrounding the cursor
         // find the beginning of the word the cursor is on
         start = min(start, ABookmark::kMaxName);
         ASSERT(start <= name.GetLength());
         int iStart=0, iEnd=0, nWords=0;
         for (iStart = start - 1; iStart > 0; --iStart)
            if (::_istspace(name[iStart]))
               break;
         for (iEnd = iStart + 1; (iEnd < name.GetLength()) && 
            (nWords < ABookmark::kDefWords); ++iEnd)
            if (_T(' ') == name[iEnd])
               ++nWords;
            else if (::_istspace(name[iEnd]))
               break;

         ASSERT(iEnd >= iStart);
         name = name.Mid(iStart, iEnd - iStart).Trim();
      }

      if (!name.IsEmpty())
      {
         dlg.fName = name;
         if (dlg.fName.GetLength() > ABookmark::kMaxName)
            dlg.fName.Truncate(ABookmark::kMaxName);
      }

      if (IDOK == dlg.DoModal())
      {
         ASSERT(!dlg.fName.IsEmpty());
         ASSERT(dlg.fName.GetLength() <= ABookmark::kMaxName);

         const int pos = this->GetCurrentUserCharPos();
         ABookmark bookmark(dlg.fName, pos);
         VERIFY(fBookmarks.Add(bookmark));
         this->ValidateMarks(true);

         fParent->InvalidateRect(this->GetCueBarRect());
         return true;
      }
   }
   else
   {
      ::AfxMessageBox(IDS_MAX_BOOKMARKS, MB_OK | MB_ICONEXCLAMATION, IDS_MAX_BOOKMARKS);
   }

   return false;
}

void ARtfHelper::BuildBookmarkMenu(CMenu& menu)
{
   for (size_t i = 0; i < fBookmarks.size(); ++i)
   {
      CString hotkey;
      if (i < 10)
         hotkey.Format(_T("\t Ctrl+%d"), (i + 1) % 10);
      else if (i < 20)
         hotkey.Format(_T("\t Ctrl+Shift+%d"), (i + 1) % 10);
      VERIFY(menu.AppendMenu(MF_STRING, rCmdGotoBkMk1 + i, 
         fBookmarks[i].fName + hotkey));
   }
}

void ARtfHelper::UpdateSetBookmark(CCmdUI *pCmdUI)
{
   if (NULL == pCmdUI->m_pMenu)
      return;

   // we don't actually do anything to the 'Set Bookmark' menu item.
   // instead, we remove the old 'Goto Bookmark' item and build the new one
   // first, remove the old one
   CMenu* gotoBookmarksMenu = pCmdUI->m_pMenu->GetSubMenu(kGotoBookmarkItemPos);
   if (NULL != gotoBookmarksMenu)
   {
      VERIFY(pCmdUI->m_pMenu->RemoveMenu(kGotoBookmarkItemPos, MF_BYPOSITION));
      VERIFY(gotoBookmarksMenu->DestroyMenu());
      gotoBookmarksMenu = NULL;
   }

   // now build the new one
   CMenu bookmarks;
   VERIFY(bookmarks.CreatePopupMenu());
   this->BuildBookmarkMenu(bookmarks);

   CString gotoBookmarksMenuTitle;
   VERIFY(gotoBookmarksMenuTitle.LoadString(rStrGotoBookmark));
   VERIFY(pCmdUI->m_pMenu->InsertMenu(kGotoBookmarkItemPos, MF_BYPOSITION | MF_STRING | MF_POPUP, 
      (UINT)bookmarks.Detach(), gotoBookmarksMenuTitle));
}

bool ARtfHelper::GotoBookmark(UINT uId)
{
   const size_t i = (uId - rCmdGotoBkMk1);
   ASSERT((i >= 0) && (i < ABookmark::kMaxCount));
   const bool valid = (i < fBookmarks.size());
   if (valid)
      this->SetCurrentUserCharPos(fBookmarks[i].fPos);

   return valid;
}

bool ARtfHelper::ClearBookmarks()
{
   ASSERT(!fScrolling);
   if (!fBookmarks.empty())
   {
      fBookmarks.clear();
      fParent->InvalidateRect(this->GetCueBarRect());
      return true;
   }

   return false;
}

void ARtfHelper::UpdateClearBookmarks(CCmdUI *pCmdUI)
{
   pCmdUI->Enable(!fBookmarks.empty());
}

bool ARtfHelper::NextBookmark()
{
   const bool valid = (!fBookmarks.empty());
   if (valid)
   {
      const int charPos = fBookmarks.Next(this->GetCurrentUserCharPos(true));
      TRACE1("GotoBookmark at char pos %d\n", charPos);
      this->SetCurrentUserCharPos(charPos);
   }
   return valid;
}

void ARtfHelper::UpdateNextBookmark(CCmdUI *pCmdUI)
{
   pCmdUI->Enable(!fBookmarks.empty());
}

bool ARtfHelper::PrevBookmark()
{
   const bool valid = (!fBookmarks.empty());
   if (valid)
   {
      const int charPos = fBookmarks.Prev(this->GetCurrentUserCharPos(false));
      TRACE1("GotoBookmark at char pos %d\n", charPos);
      this->SetCurrentUserCharPos(charPos);
   }
   return valid;
}

void ARtfHelper::UpdatePrevBookmark(CCmdUI *pCmdUI)
{
   pCmdUI->Enable(!fBookmarks.empty());
}

bool ARtfHelper::SetPaperclip()
{
   const int ch = this->GetCurrentUserCharPos();
   if (fScrolling)
   {  // in scroll mode, just add the paperclip - overwrite 
      // an existing one on this line
      fPaperclips.Add(ch);
      
      // draw the paperclip in the text area - it won't 
      // be redrawn after it's scrolled out of view
      fPaperclip.SetPosition(0, sCueMarker.GetPosition().y);
      CClientDC dc(fEdit);
      fPaperclip.Draw(dc);
   }
   else
   {  // in edit mode, toggle the paperclip
      fPaperclips.Toggle(ch);
      fParent->InvalidateRect(this->GetCueBarRect());
   }

   this->ValidateMarks(true);
   return true;
}

void ARtfHelper::UpdateSetPaperclip(CCmdUI *pCmdUI)
{
   if (NULL == pCmdUI->m_pMenu)
      return;

   // if there's already a paperclip on this line, change the
   // menu text to 'Remove Paperclip'. Otherwise, change the
   // menu text to 'Set Paperclip'.
   const long line = fEdit->LineFromChar(-1);
   const int pos = fEdit->LineIndex(line);
   AMarker paperclip(pos);
   const bool lineIsPaperclipped = (fPaperclips.end() != 
      std::find(fPaperclips.begin(), fPaperclips.end(), paperclip));

   CString text;
   VERIFY(text.LoadString(lineIsPaperclipped? IDS_DEL_PAPERCLIP : IDS_SET_PAPERCLIP));
   MENUITEMINFO info;
   ::memset(&info, 0, sizeof(info));
   info.cbSize = sizeof(info);
   info.fMask = MIIM_STRING;
   info.cch = text.GetLength();
   info.dwTypeData = (LPTSTR)(LPCTSTR)text;
   VERIFY(pCmdUI->m_pMenu->SetMenuItemInfo(pCmdUI->m_nID, &info));
}

bool ARtfHelper::ClearPaperclips()
{
   if (!fPaperclips.empty())
   {
      fPaperclips.clear();
      fParent->InvalidateRect(this->GetCueBarRect());
      return true;
   }

   return false;
}

void ARtfHelper::UpdateClearPaperclips(CCmdUI *pCmdUI)
{
   pCmdUI->Enable(!fPaperclips.empty());
}

bool ARtfHelper::NextPaperclip()
{
   const bool valid = (!fPaperclips.empty());
   if (valid)
   {
      const int charPos = fPaperclips.Next(this->GetCurrentUserCharPos(true));
      TRACE1("GotoPaperclip at char pos %d\n", charPos);
      this->SetCurrentUserCharPos(charPos);
   }
   return valid;
}

void ARtfHelper::UpdateNextPaperclip(CCmdUI *pCmdUI)
{
   pCmdUI->Enable(!fPaperclips.empty());
}

bool ARtfHelper::PrevPaperclip()
{
   const bool valid = (!fPaperclips.empty());
   if (valid)
   {
      const int charPos = fPaperclips.Prev(this->GetCurrentUserCharPos(false));
      TRACE1("GotoPaperclip at char pos %d\n", charPos);
      this->SetCurrentUserCharPos(charPos);
   }
   return valid;
}

void ARtfHelper::UpdatePrevPaperclip(CCmdUI *pCmdUI)
{
   pCmdUI->Enable(!fPaperclips.empty());
}

CRect ARtfHelper::GetCueBarRect() const
{
   ASSERT(NULL != fEdit);
   ASSERT(NULL != fParent);

   CRect r;
   if (fScrolling)
   {
      fEdit->GetWindowRect(r);
      fParent->ScreenToClient(r);
      r.SetRect(r.left - kCueBarWidth, r.top - kRtfVOffset, r.left, r.bottom);
   }
   else
   {
      fEdit->GetRect(r);
      r.SetRect(0, r.top - kRtfVOffset, kCueBarWidth, r.bottom);
   }

   return r;
}

void ARtfHelper::PaintCueBar()
{
   CClientDC dc(fParent);
   CRect cueBar = this->GetCueBarRect();
   dc.IntersectClipRect(cueBar);

   CRect update;
   if (fParent->GetUpdateRect(update))
      dc.IntersectClipRect(update);

   // fill the background
   int line = fEdit->SendMessage(EM_GETFIRSTVISIBLELINE);
   CPoint pos = fEdit->PosFromChar(fEdit->LineIndex(line));
   const int nLines = fEdit->GetLineCount();
   dc.FillSolidRect(cueBar, (fScrolling && theApp.fInverseOut) ? 
      kBlack : (fScrolling ? kWhite : GetSysColor(COLOR_BTNFACE)));

   if (!fScrolling)
   {
      dc.DrawEdge(cueBar, EDGE_RAISED, BF_RIGHT);

      CPen pen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNSHADOW));
      dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
      dc.SelectObject(::GetStockObject(DEFAULT_GUI_FONT));
      dc.SetBkMode(TRANSPARENT);
      dc.SelectObject(pen);

      const CPoint topPos = pos;
      while ((pos.y <= cueBar.bottom) && (line < nLines))
      {
         // calculate the rectangle for this line
         const int chLineStart = fEdit->LineIndex(line);
         const int chNextLineStart = (++line == nLines) ? INT_MAX : fEdit->LineIndex(line);
         const CPoint nextPos = (line == nLines) ? 
            CPoint(0, cueBar.bottom) : fEdit->PosFromChar(chNextLineStart);
         CRect lineRect(cueBar.left, pos.y-1, cueBar.left + kCueBarWidth, nextPos.y);
         pos = nextPos;

         // draw markers
         if (fPaperclips.FindOnLine(chLineStart, chNextLineStart - 1))
         {
            fPaperclip.SetPosition(lineRect.left + 1, 
               lineRect.CenterPoint().y - fPaperclip.GetSize().cy / 2 - 3);
            fPaperclip.Draw(dc);
         }
         if (fBookmarks.FindOnLine(chLineStart, chNextLineStart - 1))
         {
            fBookmark.SetPosition(lineRect.left + 1, 
               lineRect.CenterPoint().y - fBookmark.GetSize().cy / 2 + 3);
            fBookmark.Draw(dc);
         }

         if (theApp.fLineNumbers)
         {  // draw separator line
            lineRect.DeflateRect(4, 0);
            VERIFY(::DrawEdge(dc, lineRect, EDGE_ETCHED, BF_BOTTOM));

            // draw line number
            CString text;
            text.Format(_T("%d"), line);
            lineRect.InflateRect(4, 0);
            lineRect.right -= lineRect.Width() / 3;
            dc.DrawText(text, lineRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
         }
      }
   }

   if (!fScrolling)
      sCueMarker.SetImage(rIcoCueBar0Alt + theApp.fCuePointer + 
         ((fScrolling && theApp.fInverseOut) ? 5 : 0));
   else if (1 == theApp.fCuePointer) // OFF
      sCueMarker.SetImage(0);
   else
      sCueMarker.SetImage(rIcoCueBar0 + theApp.fCuePointer + 
         (theApp.fInverseOut ? 5 : 0));

   sCueMarker.Draw(dc, cueBar.top);
}

void ARtfHelper::Paint()
{
   CRect update;
   fParent->GetUpdateRect(update);
   CRect cueBar = this->GetCueBarRect();
   if (update.IntersectRect(update, cueBar))
   {
      this->PaintCueBar();
      fParent->ValidateRect(update);
   }
}

void ARtfHelper::Size(int x, int y, int cx, int cy)
{  // the coordinates passed in are the client coordinates
   // of the rich edit control.
   CPoint cuePos(x - (sCueMarker.GetSize().cx + 2), -1);
   if (sCueMarker.GetPosition().y < 0)
   {  // uninitialized. Read the position from the registry
      cuePos.y = theApp.GetProfileInt(gStrSection, gStrCuePos, 
         y + kRtfVOffset + (cy / 4));
   }

   // set the rich edit control display area
   CRect r;
   cx = (theApp.fScrollMarginRight - theApp.fScrollMarginLeft);
   if (fScrolling)
   {
      const int left = theApp.fScrollMarginLeft + kRtfHOffset;
	   r = CRect(CPoint(left, y + kRtfVOffset), CSize(cx, cy));
   }
   else
   {
      // CRichEditView::SetRect actually decreases area by 1
      const int left = theApp.fScrollMarginLeft + kCueBarWidth + kRtfHOffset + 1;
	   r = CRect(CPoint(left, y + kRtfVOffset), CSize(cx - 2, cy));
      cuePos.x += kCueBarWidth;
   }

   fEdit->SetRect(r);
   mDebugOnly(fEdit->GetRect(r));

   sCueMarker.SetPosition(cuePos);
}

void ARtfHelper::SetDefaultFont()
{
	fEdit->SetSel(0,-1);
	fEdit->SetDefaultCharFormat(theApp.fDefaultFont);
	fEdit->SetSelectionCharFormat(theApp.fDefaultFont);
	fEdit->SetSel(0,0);
	fEdit->EmptyUndoBuffer();
	fEdit->SetModify(FALSE);
}

bool ARtfHelper::Dragging() const
{
   ASSERT(!fScrolling);
   return fDragging;
}

bool ARtfHelper::StartDrag(const CPoint& point)
{
   ASSERT(!fScrolling);
   CRect cueBar = this->GetCueBarRect();
   if (!fDragging && sCueMarker.GetRect(cueBar.top).PtInRect(point))
   {
      fParent->SetCapture();
      fParent->ClientToScreen(cueBar);
      VERIFY(::ClipCursor(cueBar));

      fDragPoint = point;
      fDragDelta = fDragPoint.y - sCueMarker.GetPosition().y;
      fDragging = true;
      //TRACE2("Start dragging at (%d,%d)\n", point.x, point.y);
   }

   return fDragging;
}

void ARtfHelper::Drop(const CPoint&)
{
   ASSERT(!fScrolling);
   if (fDragging)
   {
      VERIFY(::ClipCursor(NULL));
      VERIFY(::ReleaseCapture());
      fDragging = false;

      // save the position
      VERIFY(theApp.WriteProfileInt(gStrSection, gStrCuePos, 
         sCueMarker.GetPosition().y));

      //TRACE2("Drop at (%d,%d)\n", point.x, point.y);
   }
}

void ARtfHelper::Drag(const CPoint& point)
{
   ASSERT(!fScrolling);
   if (fDragging && (fDragPoint.y != point.y))
   {  // erase the old cue marker
      CRect cueBar(this->GetCueBarRect());
      CRect cueMarker(sCueMarker.GetRect(cueBar.top));
      fParent->InvalidateRect(cueMarker);

      fDragPoint = point;
      sCueMarker.SetPosition(cueMarker.left, fDragPoint.y - fDragDelta);
      fParent->InvalidateRect(sCueMarker.GetRect(cueBar.top));
      //TRACE2("Drag to (%d,%d)\n", point.x, point.y);
   }
}

void ARtfHelper::CancelDrag()
{
   ASSERT(!fScrolling);
   VERIFY(::ClipCursor(NULL));
   VERIFY(::ReleaseCapture());
   fDragging = false;
}

void ARtfHelper::ValidateMarks(bool dump)
{
#ifdef _DEBUG
   if (NULL != fEdit)
   {
      for (APaperclips::iterator i = fPaperclips.begin(); i != fPaperclips.end(); ++i)
      {  // make sure each paperclip is at the start of a line
         const long line = fEdit->LineFromChar(i->fPos);
         ASSERT(i->fPos == fEdit->LineIndex(line));
      }
   }

   if (dump)
   {
      TRACE("Bookmarks: "); fBookmarks.Dump(); TRACE("\n");
      TRACE("Paperclips: "); fPaperclips.Dump(); TRACE("\n");
   }
#else
   dump;
#endif // _DEBUG
}
