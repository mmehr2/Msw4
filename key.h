// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


#include "Msw.h"

// A simple registry key manipulation class
class AKey
{
public:
   AKey() : fKey(NULL) {}
	virtual ~AKey() {this->Close();}

public:
   bool IsValid() const;

   bool Create(HKEY hKey, LPCTSTR lpszKeyName);
	bool Open(HKEY hKey, LPCTSTR lpszKeyName);
	void Close();

   bool SetStringValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
	bool GetStringValue(CString& str, LPCTSTR lpszValueName = NULL);

   bool SetDwordValue(DWORD dwValue, LPCTSTR lpszValueName);
   bool GetDwordValue(DWORD& dwValue, LPCTSTR lpszValueName);

private:
	HKEY fKey;
};

//-----------------------------------------------------------------------------
// inline functions
inline bool AKey::IsValid() const
{
   return (NULL != fKey);
}