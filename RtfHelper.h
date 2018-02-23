// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "bookmark.h"
#include "ruler.h"
#include "Timer.h"

class AGuiMarker
{
public:
   AGuiMarker(UINT resId, const CSize& size);
   void Draw(CDC& dc, int verticalOffset = 0);
   void SetPosition(int x, int y);
   void SetPosition(const CPoint& pos);
   CPoint GetPosition() const;
   CSize GetSize() const;
   CRect GetRect(int verticalOffset) const;
   void SetImage(UINT resId);

private:
   UINT fResId;
   const CSize fSize;
   CPoint fPosition;
   HICON fIcon;
};


class ARtfHelper
{
public:
   static const int kRtfHOffset  = 10;
   static const int kRtfVOffset  = 0;
   static const int kCueBarWidth = 34;

   ARtfHelper(bool scrolling);

   bool GotoBookmark(UINT uId);
   bool SetBookmark();
   void BuildBookmarkMenu(CMenu& menu);
   void UpdateSetBookmark(CCmdUI *pCmdUI);
   bool ClearBookmarks();
   void UpdateClearBookmarks(CCmdUI *pCmdUI);
   bool NextBookmark();
   void UpdateNextBookmark(CCmdUI *pCmdUI);
   bool PrevBookmark();
   void UpdatePrevBookmark(CCmdUI *pCmdUI);

   bool SetPaperclip();
   void UpdateSetPaperclip(CCmdUI *pCmdUI);
   bool ClearPaperclips();
   void UpdateClearPaperclips(CCmdUI *pCmdUI);
   bool NextPaperclip();
   void UpdateNextPaperclip(CCmdUI *pCmdUI);
   bool PrevPaperclip();
   void UpdatePrevPaperclip(CCmdUI *pCmdUI);
   void ValidateMarks(bool dump);

   void Size(int x, int y, int cx, int cy);
   void SetDefaultFont();
   void Paint();

   bool Dragging() const;
   bool StartDrag(const CPoint& point);
   void Drop(const CPoint& point);
   void Drag(const CPoint& point);
   void CancelDrag();
   CRect GetCueBarRect() const;

   // Return the pixel postition at the cue pointer
   int GetCurrentUserPos();
   void SetCurrentUserPos(int pos);
   CPoint GetPosFromChar(int chPos);
   unsigned short GetCharAt(int charPos, int& csize);
   long GetTextLen() const;

   // Return the character postition at either the cue pointer or
   // the cursor. 'Max' can be used to specify the horizontal right
   // end of the line when asking the rich text control for the 
   // position.
   int GetCurrentUserCharPos(bool max = false);
   void SetCurrentUserCharPos(int chPos);

private:
   static const int kGotoBookmarkItemPos  = 1;

   void PaintCueBar();

public:
   CWnd* fParent;
   CRichEditCtrl* fEdit;
   ABookmarks fBookmarks;
   APaperclips fPaperclips;
   bool fScrolling;
   bool fDragging;
   CPoint fDragPoint;
   int fDragDelta;
   ATimer fArt;
   static AGuiMarker sCueMarker;
   AGuiMarker fPaperclip;
   AGuiMarker fBookmark;

private:
	CCharFormat m_defCharFormat;
	CCharFormat m_defTextCharFormat;
};

//-----------------------------------------------------------------------
// inline functions
inline AGuiMarker::AGuiMarker(UINT resId, const CSize& size) :
   fResId(resId),
   fSize(size),
   fPosition(-1, -1),
   fIcon(NULL)
{
}

inline void AGuiMarker::SetPosition(const CPoint& pos)
{
   this->SetPosition(pos.x, pos.y);
}

inline CPoint AGuiMarker::GetPosition() const
{
   return fPosition;
}

inline CSize AGuiMarker::GetSize() const
{
   return fSize;
}

inline void AGuiMarker::SetImage(UINT resId)
{
   if (resId != fResId)
   {
      fResId = resId;
      fIcon = NULL;
   }
}
