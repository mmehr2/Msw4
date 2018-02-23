// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "key.h"
#include <winreg.h>

void AKey::Close()
{
	if (fKey != NULL)
	{
      LONG lRes = ::RegCloseKey(fKey);
		VERIFY(lRes == ERROR_SUCCESS);
		fKey = NULL;
	}
}

bool AKey::Create(HKEY key, LPCTSTR lpszKeyName)
{
	ASSERT(key != NULL);
   return (::RegCreateKey(key, lpszKeyName, &fKey) == ERROR_SUCCESS);
}

bool AKey::Open(HKEY key, LPCTSTR lpszKeyName)
{
	ASSERT(key != NULL);
   return (::RegOpenKey(key, lpszKeyName, &fKey) == ERROR_SUCCESS);
}

bool AKey::SetStringValue(LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	ASSERT(fKey != NULL);
   return (::RegSetValueEx(fKey, lpszValueName, NULL, REG_SZ,
		(BYTE * const)lpszValue, (lstrlen(lpszValue)+1)*sizeof(TCHAR)) == ERROR_SUCCESS);
}

bool AKey::GetStringValue(CString& str, LPCTSTR lpszValueName)
{
	ASSERT(fKey != NULL);
	str.Empty();
	DWORD dw = 0;
	DWORD dwType = 0;
   LONG lRes = ::RegQueryValueEx(fKey, (LPTSTR)lpszValueName, NULL, &dwType, NULL, &dw);
	if (lRes == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_SZ);
		LPTSTR lpsz = str.GetBufferSetLength(dw);
      lRes = ::RegQueryValueEx(fKey, (LPTSTR)lpszValueName, NULL, &dwType, (BYTE*)lpsz, &dw);
		ASSERT(lRes == ERROR_SUCCESS);
		str.ReleaseBuffer();
		return TRUE;
	}
	return FALSE;
}

bool AKey::SetDwordValue(DWORD dwValue, LPCTSTR lpszValueName)
{
	ASSERT(fKey != NULL);
   return (::RegSetValueEx(fKey, lpszValueName, NULL, REG_DWORD,
		(BYTE*)&dwValue, sizeof(dwValue)) == ERROR_SUCCESS);
}

bool AKey::GetDwordValue(DWORD& dwValue, LPCTSTR lpszValueName)
{
	ASSERT(fKey != NULL);
	DWORD dw = 0;
	DWORD dwType = 0;
   LONG lRes = ::RegQueryValueEx(fKey, (LPTSTR)lpszValueName, NULL, &dwType, NULL, &dw);
	if ((lRes == ERROR_SUCCESS) && (REG_DWORD == dwType))
	{
      lRes = ::RegQueryValueEx(fKey, (LPTSTR)lpszValueName, NULL, &dwType, 
         (BYTE*)&dwValue, &dw);
		ASSERT(lRes == ERROR_SUCCESS);
		return TRUE;
	}
	return FALSE;
}
