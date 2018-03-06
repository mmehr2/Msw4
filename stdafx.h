// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#define WINVER 0x0500

#ifndef _UNICODE
#  define VC_EXTRALEAN        // use stripped down Win32 headers
#endif

#define CONVERTERS

#define _AFX_ALL_WARNINGS
#include <afxwin.h>        // MFC core and standard components
#include <afxext.h>        // MFC extensions
#include <afxole.h>        // MFC OLE classes
#include <afxodlgs.h>      // MFC OLE dialog classes
#include <afxcmn.h>
#include <afxrich.h>
#include <afxpriv.h>
#include <afxdtctl.h>      // MFC support for Internet Explorer 4 Common Controls

#ifdef _UNICODE
#  if defined _M_IX86
#     pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#  else
#     pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#  endif
#endif

//-----------------------------------------------------------------------
// voice recognition
//#pragma warning (push)
//#pragma warning (disable: 4996)  // 'wcscpy' was declared deprecated
//#include <sapi.h>
//#include <sphelper.h>
//#pragma warning (pop)

#define DATE_DIRECT			// tells program not to use date fudge

//-----------------------------------------------------------------------
struct CCharFormat : public CHARFORMAT2
{
   CCharFormat() {::memset(this, 0, sizeof(CHARFORMAT2)); cbSize = sizeof(CHARFORMAT2);}
   CCharFormat(const CHARFORMAT2& cf) {*this = cf;}
   CCharFormat& operator = (const CHARFORMAT& cf) {::memcpy(this, &cf, sizeof(cf)); this->cbSize = sizeof(CHARFORMAT2); return *this;}
   bool operator == (const CCharFormat& cf) const {return (*this == (CHARFORMAT2&)cf);}
   bool operator == (const CHARFORMAT2& cf) const;
   void SetFaceName(LPCTSTR name);
};

struct CParaFormat : public PARAFORMAT2
{
	CParaFormat() {::memset(this, 0, sizeof(PARAFORMAT2)); cbSize = sizeof(PARAFORMAT2);}
   CParaFormat(const PARAFORMAT2& pf) {*this = pf;}
   CParaFormat& operator = (const PARAFORMAT2& pf) {::memcpy(this, &pf, sizeof(pf)); this->cbSize = sizeof(PARAFORMAT2); return *this;}
	bool operator == (PARAFORMAT2& pf);
};

#define mCountOf(a) (sizeof(a) / sizeof(a[0]))

#ifdef _DEBUG
#  define mDebugOnly(e) (e)
#else // !_DEBUG
#  define mDebugOnly(e)
#endif // _DEBUG


#include "Utils.h"
#include <afxdlgs.h>
#include <afxsock.h>
