// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "ScriptQueue.h"
#include "Msw.h"
#include "MswDoc.h"
#include "MswView.h"
#include "MainFrm.h"

#include <algorithm>

AScript::AScript(AMswView* view) :
   fScroll(true),
   fView(view),
   fStart(0),
   fEnd(0)
{
}

CString AScript::GetPath() const
{
   ASSERT(NULL != fView);
   return (NULL == fView) ? CString() : fView->GetDocument()->GetPathName();
}

CString AScript::GetName() const
{
   CString path = this->GetPath();
   TCHAR name[_MAX_FNAME];
   ::_tsplitpath_s(path, NULL, 0, NULL, 0, name, mCountOf(name), NULL, 0);
   return name;
}

DWORD AScript::GetErt() const
{
   ASSERT(NULL != fView);
   return (NULL == fView) ? 0 : fView->GetErt();
}

DWORD AScript::GetArt() const
{
   ASSERT(NULL != fView);
   return (NULL == fView) ? 0 : fView->GetArt();
}

#ifdef _DEBUG
void AScript::Dump() const
{
   TRACE(_T("AScript: %s {%d,%d}\n"), (LPCTSTR)this->GetName(), 
      this->GetStart(), this->GetEnd());
}
#endif // _DEBUG


bool AScriptQueue::Add(const AScript& script)
{
   iterator i = std::find(this->begin(), this->end(), script);
   if (this->end() == i)
   {
      this->push_back(script);
      return true;
   }

   return false;
}

bool AScriptQueue::Remove(const AScript& script)
{
   iterator i = std::find(this->begin(), this->end(), script);
   if (this->end() != i)
   {
      this->erase(i);
      return true;
   }

   return false;
}

size_t AScriptQueue::GetActive() const
{
   CMDIChildWnd* child = ((CMDIFrameWnd*)::AfxGetMainWnd())->MDIGetActive();
   if (NULL != child)
   {
      CView* view = child->GetActiveView();
      if (NULL != view)
         for (size_t i = 0; i < this->size(); ++i)
            if (view == (*(const_cast<AScriptQueue*>(this)))[i].GetView())
               return i;
   }

   return (this->size() + 1);
}

AScriptQueue AScriptQueue::GetQueuedScripts()
{
   AScriptQueue queued;

   const size_t active = this->GetActive();
   if (active < this->size())
   {
      if (!theApp.fLink)
      {  // just get one script. Note that if linking is off, we'll get the
         // active document or the first scrollable one after the active one.
         size_t iScript = this->size();
         for (size_t i = 0; i < this->size(); ++i)
            if (((*this)[i].GetScroll()) &&
               ((iScript >= this->size()) || (active == i)))
               iScript = i;   // this is the first scrollable script or the active one

         if ((iScript < this->size()) && ((*this)[iScript].GetScroll()))
            queued.Add((*this)[iScript]);
      }
      else for (size_t i = 0; i < this->size(); ++i)
         if ((*this)[i].GetScroll())
            queued.Add((*this)[i]);
   }

   return queued;
}

AScript AScriptQueue::GetScriptFromPos(int pos) const
{
   for (const_iterator i = this->begin(); i != this->end(); ++i)
      if (pos < i->GetEnd())
         return *i;

   return (this->size() > 0) ? *this->rbegin() : AScript();
}

void AScriptQueue::Activate(size_t i)
{
   ASSERT((i >= 0) && (i < this->size()));
   if ((i >= 0) && (i < this->size()))
   {
      AMainFrame* frame = dynamic_cast<AMainFrame*>(::AfxGetMainWnd());
      if (NULL != frame)
         frame->MDIActivate((*this)[i].GetView()->GetParent());
   }
}


#ifdef _DEBUG
void AScriptQueue::Dump() const
{
   TRACE(_T("AScriptQueue:\n"));
   const size_t active = this->GetActive();
   for (size_t i = 0; i < this->size(); ++i)
   {
      TRACE(_T("\t%s"), (active == i) ? _T("*") : _T(""));
      (*this)[i].Dump();
   }
}
#endif // _DEBUG
