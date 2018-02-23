// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "Utils.h"

bool Utils::FileExists(const CString& strPath)
{
   WIN32_FIND_DATA find = {0};
   const HANDLE hFile = ::FindFirstFile(strPath, &find);
   if (INVALID_HANDLE_VALUE != hFile)
      ::FindClose(hFile);

   return (INVALID_HANDLE_VALUE != hFile);
}

long Utils::GetTextLenth(HWND& edit)
{
   GETTEXTLENGTHEX gtlx = {GTL_PRECISE, CP_ACP};
   return (long)::SendMessage(edit, EM_GETTEXTLENGTHEX, (WPARAM)&gtlx, 0); 
}

CString Utils::FormatDuration(int hours, int minutes, int seconds)
{
   CString text;
   text.Format(_T("%01d:%02d:%02d"), hours, minutes, seconds);
   return text;
}

CString Utils::GetErrorText(DWORD err)
{
   CString text;
   LPTSTR buffer = NULL;
   if (0 != ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
      NULL, err, 0, (LPTSTR)&buffer, 0, NULL))
   { // success
      text = buffer;
      ::LocalFree(buffer);
   }

   return text;
}
