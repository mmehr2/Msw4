// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

static const BYTE sRtfPrefix[]      = {'{', '\\', 'r', 't', 'f'};
static const wchar_t sURtfPrefix[]  = {0xFFFE, L'{', L'\\', L'r', L't', L'f'};
static const BYTE sUTxt[]           = {0xFF, 0xFE};
static const BYTE sWord2Prefix[]    = {0xDB, 0xA5, 0x2D, 0x00};
static const BYTE sCompFilePrefix[] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
static const BYTE sExePrefix[]      = {0x4D, 0x5A};

static const COLORREF kWhite = RGB(0xff, 0xff, 0xff);
static const COLORREF kBlack = RGB(0, 0, 0);

class CRichEditCtrl;
class CRtfMemFile;
class CFile;

namespace Utils
{
   bool FileExists(const CString& strPath);
   long GetTextLenth(HWND& edit);
   CString FormatDuration(int hours, int minutes, int seconds);
   inline CString FormatDuration(DWORD sec) 
      {return Utils::FormatDuration(sec / 3600, (sec % 3600) / 60, (sec % 60));}

   CString GetErrorText(DWORD err);
} // namespace Utils
