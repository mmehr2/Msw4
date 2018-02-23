// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include <vector>


class AMswView;

class AScript
{
public:
   AScript(AMswView* view = NULL);

   CString GetPath() const;
   CString GetName() const;
   DWORD GetErt() const;
   DWORD GetArt() const;
   bool GetScroll() const;
   void SetScroll(bool scroll);
   AMswView* GetView();

   // get the offsets of this script in the queue
   long GetStart() const;
   void SetStart(long start);
   long GetEnd() const;
   void SetEnd(long end);

   bool operator == (const AScript& script) const;

#ifdef _DEBUG
   virtual void Dump() const;
#endif // _DEBUG

private:
   bool fScroll;
   AMswView* fView;
   long fStart, fEnd;   // offsets of this script in the queue
};

class AScriptQueue : public std::vector<AScript>
{
public:
   AScriptQueue() {}

   // Add a script to the queue. If the script is already in the queue,
   // it won't be added again.
   bool Add(const AScript& script);

   // Remove a script from the queue. Return false if the script was
   // not found
   bool Remove(const AScript& script);

   // Return a queue of scripts to be scrolled. If linking is off, the 
   // active script or the first scrollable script after the active one
   // is returned.
   AScriptQueue GetQueuedScripts();

   // Return the index of the currently 'active' script. The active script
   // is the one in the foreground of the edit view. If there is no active
   // script, size() + 1 is returned.
   size_t GetActive() const;

   // Return the script at location 'pos' in the queue.
   AScript GetScriptFromPos(int pos) const;

   // Activate the specified script.
   void Activate(size_t i);

#ifdef _DEBUG
   virtual void Dump() const;
#endif // _DEBUG
};

///////////////////////////////////////////////////////////////////////////////
// inline functions
inline bool AScript::operator == (const AScript& script) const
{
   return this->GetPath() == script.GetPath();
}

inline bool AScript::GetScroll() const
{
   return fScroll;
}

inline void AScript::SetScroll(bool scroll)
{
   fScroll = scroll;
}

inline AMswView* AScript::GetView()
{
   return fView;
}

inline long AScript::GetStart() const
{
   return fStart;
}

inline void AScript::SetStart(long start)
{
   fStart = start;
}

inline long AScript::GetEnd() const
{
   return fEnd;
}

inline void AScript::SetEnd(long end)
{
   fEnd = end;
}
