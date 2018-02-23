// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include <vector>
#include <algorithm>


class AMarker
{
public:
   AMarker(int pos = 0);
   virtual bool operator == (const AMarker& marker) const;
   virtual bool operator < (const AMarker& marker) const;

#ifdef _DEBUG
   virtual void Dump() const;
#endif // _DEBUG

public:
   int fPos;
};

template <class T>
class AMarkers : public std::vector<T>
{
public:
   AMarkers();
   bool Add(const T& marker);
   bool Toggle(const T& marker);
   void Adjust(long len, long delta, long pos, CRichEditCtrl& ed);

   int Next(int cur);
   int Prev(int cur);

   bool FindOnLine(int chStart, int chEnd);

#ifdef _DEBUG
   void Dump() const;
#endif // _DEBUG

private:
   size_t fCurrent;
};


/** Bookmarks have a name, start, and end positions in the document
    They are sorted by start position.
 */
class ABookmark : public AMarker
{
public:
   static const short kMinName = 1;
   static const short kMaxName = 30;
   static const short kMaxCount = 100;
   static const short kDefWords = 3;

   ABookmark(const CString& name, int pos = 0);
   bool operator == (const ABookmark& bookmark) const;

#ifdef _DEBUG
   virtual void Dump() const;
#endif // _DEBUG

public:
   CString fName;
};

typedef AMarkers<ABookmark> ABookmarks;
typedef AMarkers<AMarker> APaperclips;


//-----------------------------------------------------------------------
// inline functions
inline AMarker::AMarker(int pos) :
   fPos(pos)
{
}

inline bool AMarker::operator < (const AMarker& marker) const
{
   return (fPos < marker.fPos);
}

inline bool AMarker::operator == (const AMarker& marker) const
{
   return (fPos == marker.fPos);
}

#ifdef _DEBUG
inline void AMarker::Dump() const
{
   TRACE1("%d", fPos);
}
#endif // _DEBUG


template <class T>
inline AMarkers<T>::AMarkers() :
   fCurrent(0)
{
}

template <class T>
inline bool AMarkers<T>::Add(const T& marker)
{
   iterator i = std::find(this->begin(), this->end(), marker);
   if (this->end() == i)
      this->push_back(marker);
   else
      *i = marker; // overwrite
   std::sort(this->begin(), this->end());

   return true;
}

template <class T>
inline bool AMarkers<T>::Toggle(const T& marker)
{
   iterator i = std::find(this->begin(), this->end(), marker);
   if (this->end() != i)
      this->erase(i);
   else
   {
      this->push_back(marker);
      std::sort(this->begin(), this->end());
   }

   return true;
}

template <class T>
inline void AMarkers<T>::Adjust(long len, long delta, long pos, CRichEditCtrl& ed)
{
   if (0 == delta)
      return;  // nothing to do

   for (reverse_iterator i = rbegin(); i != rend(); ++i)
   {
      if (i->fPos >= (pos - delta))
      {
         i->fPos += delta;

         // now make sure our marker is on the first character of its line
         i->fPos = ed.LineIndex(ed.LineFromChar(i->fPos));
      }
   }

   // remove all bookmarks that are past the end of the document
   while (!empty() && (rbegin()->fPos > len))
      this->pop_back();
}

#ifdef _DEBUG
template <class T>
inline void AMarkers<T>::Dump() const
{
   for (const_iterator i = begin(); i != end(); ++i)
   {
      if (begin() != i)
         TRACE(",");
      i->Dump();
   }
}
#endif // _DEBUG

template <class T>
inline int AMarkers<T>::Next(int cur)
{
   ASSERT(!this->empty());
   for (iterator i = begin(); i != end(); ++i)
      if (i->fPos > cur)
         return i->fPos;

   return begin()->fPos;
}

template <class T>
inline int AMarkers<T>::Prev(int cur)
{
   ASSERT(!this->empty());
   for (reverse_iterator i = rbegin(); i != rend(); ++i)
      if (i->fPos < cur)
         return i->fPos;

   return rbegin()->fPos;
}

template <class T>
inline bool AMarkers<T>::FindOnLine(int chStart, int chEnd)
{
   for (const_iterator i = begin(); i != end(); ++i)
      if ((i->fPos >= chStart) && (i->fPos <= chEnd))
         return true;

   return false;
}


inline ABookmark::ABookmark(const CString& name, int pos) :
   AMarker(pos),
   fName(name)
{  // check for illegal characters
   ASSERT(fName.FindOneOf(_T("{}")) < 0);
   fName.Remove(_T('{'));
   fName.Remove(_T('}'));
}

inline bool ABookmark::operator == (const ABookmark& bookmark) const
{
   return (0 == bookmark.fName.CompareNoCase(fName));
}

#ifdef _DEBUG
inline void ABookmark::Dump() const
{
   TRACE1("%s:", (LPCTSTR)fName);
   AMarker::Dump();
}
#endif // _DEBUG
