// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include <vector>


class AStyle
{
public:
   enum {kMaxCount = 100};

   CString fName;
   CCharFormat fFormat;
   bool operator == (const AStyle& style) {return (fFormat == style.fFormat);}
};


class AStyleList : public std::vector<AStyle>
{
public:
   AStyle* GetStyle(LPCTSTR name);
   int GetIndex(LPCTSTR name) const;
   bool Remove(LPCTSTR name);

   bool Read();
   bool Write();
};