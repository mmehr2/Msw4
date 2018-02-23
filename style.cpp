// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"

#include "msw.h"
#include "style.h"
#include "strings.h"


AStyle* AStyleList::GetStyle(LPCTSTR name)
{
   ASSERT(NULL != name);
   AStyle* result = NULL;
   for (iterator i = this->begin(); (i != this->end()) && (NULL == result); ++i)
   {
      if (i->fName == name)
      {
         result = &(*i);
      }
   }

   return result;
}

int AStyleList::GetIndex(LPCTSTR name) const
{
   ASSERT(NULL != name);
   for (size_t i = 0; i < this->size(); ++i)
   {
      if ((*this)[i].fName == name)
      {
         return i;
      }
   }

   return -1;
}

bool AStyleList::Remove(LPCTSTR name)
{
   ASSERT(NULL != name);
   for (iterator i = this->begin(); i != this->end(); ++i)
   {
      if (i->fName == name)
      {
         this->erase(i);
         return true;
      }
   }

   return false;
}

bool AStyleList::Read()
{
   this->clear();
   for (DWORD i = 0; i < UINT_MAX; ++i)
   {
      CString styleSection;
      styleSection.Format(_T("%s%d"), gStrStyles, i);

      AStyle style;
      style.fName = theApp.GetProfileString(styleSection, gStrStyleName);

      UINT nBytes = 0;
      BYTE* format = NULL;
      if (!style.fName.IsEmpty() && 
         theApp.CWinApp::GetProfileBinary(styleSection, gStrStyleFormat, &format, &nBytes) &&
         (nBytes == sizeof(style.fFormat)))
      {
         style.fFormat = *(CCharFormat*)format;
         delete [] format;
         gStyles.push_back(style);
      }
      else
         break;
   }

   return true;
}

bool AStyleList::Write()
{
   bool result = true;
   for (size_t i = 0; i < UINT_MAX; ++i)
   {
      CString styleSection;
      styleSection.Format(_T("%s%d"), gStrStyles, i);

      BYTE* junk = 0;
      UINT nBytes = 0;
      if (i < this->size())
      {  // write this format
         AStyle& style = (*this)[i];
         if (!theApp.WriteProfileString(styleSection, gStrStyleName, style.fName) ||
             !theApp.WriteProfileBinary(styleSection, gStrStyleFormat, (LPBYTE)&style.fFormat, 
             sizeof(style.fFormat)))
            result = false;
      }
      else if (theApp.CWinApp::GetProfileBinary(styleSection, gStrStyleFormat, &junk, &nBytes))
      {  // empty this one
         junk[0] = 0;
         theApp.WriteProfileString(styleSection, gStrStyleName, _T(""));
         theApp.WriteProfileBinary(styleSection, gStrStyleFormat, junk, sizeof(junk[0]));
         delete [] junk;
      }
      else
         break;   // done
   }
   return result;
}
