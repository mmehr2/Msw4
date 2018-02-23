// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "msw.h"
#include "strings.h"

#ifdef REGISTER
#  include "RegisterDlg.h"
#  include "EvalDialog.h"
#  include "UnregisteredVersionDlg.h"
#else // !REGISTER
#  include "Key.h"
#endif

#define MAGIC_DEVID     0x4B09
#define OVR_PASSWORD1   0xF86D
#define OVR_PASSWORD2   0x2600
#define WR_PASSWORD     0x695E

//-----------------------------------------------------------------------------
// local functions
bool EvalExpired();
bool TrialExpired();

bool AMswApp::InitProtection()
{
   bool initialized = true;

#ifdef REGISTER
   // check registration information
   fReg.SetInfo(this->GetProfileString(gStrSection, gStrRegName),
      this->GetProfileString(gStrSection, gStrRegNumber));
#elif 0 // !REGISTER
   // check the registry for the number of times we've run
   AKey key;
   CString kValueName = _T("MRUDockBotPos");
   CString kKeyName = CString(_T("Software\\Magic Teleprompting\\MagicScroll\\BarState-BarO"));
   static const DWORD kInitValue = 30;
   initialized = false;
   if (key.Create(HKEY_CURRENT_USER, kKeyName))
   {  // the key already exists; just decrement the count
      DWORD value = 0;
      if (!key.GetDwordValue(value, kValueName))
         value = kInitValue;

      if ((value > 0) && (value <= kInitValue))
         // write the value to the registry
         initialized = key.SetDwordValue(--value, kValueName);
   }
#endif

   return initialized;
}

bool AMswApp::IsLegalCopy()
{
   bool isLegal = true;

#ifdef REGISTER
#ifdef _EXPIRE
   if (this->EvalExpired())
      isLegal = false;
#else
   if (::TrialExpired())
      isLegal = false;
#endif // _EXPIRE

   UINT reason = 0;
   if (!theApp.fReg.IsValid(reason) && !theApp.fReg.IsTrial())
   {
      UINT reason = 0;
      ARegisterDlg dlg(theApp.fReg.GetName(), theApp.fReg.GetNumber());
      if (IDOK != dlg.DoModal())
         isLegal = false;

      this->SetRegistration(dlg.fReg.GetName(), dlg.fReg.GetNumber());
      if (!theApp.fReg.IsValid(reason))
         isLegal = false;
   }
#endif // REGISTER

   return isLegal;
}

void AMswApp::SetRegistration(LPCTSTR name, LPCTSTR number)
{
#ifdef REGISTER
   fReg.SetInfo(name, number);

   this->WriteProfileString(gStrSection, gStrRegName, fReg.GetName());
   this->WriteProfileString(gStrSection, gStrRegNumber, fReg.GetNumber());
#else
   UNUSED_ALWAYS(name);
   UNUSED_ALWAYS(number);
#endif // REGISTER
}

bool EvalExpired()
{  // this (beta, alpha) version will expire - figure out when
   bool quit = false;
#ifdef REGISTER
   DATE expire = 0;
   const COleDateTime today = COleDateTime::GetCurrentTime();
   if (theApp.GetProfileBinary(gStrPrompt, &expire, sizeof(expire)))
      quit = (today > COleDateTime(expire));
   else
   {  // set expire date to 30 days from today
      expire = COleDateTime(today + COleDateTimeSpan(30, 0, 0, 0));
      theApp.WriteProfileBinary(gStrSection, gStrPrompt, (LPBYTE)&expire, sizeof(expire));
   }

   AEvalDialog dlg;
   dlg.fNotice.FormatMessage(quit ? rStrExpired : rStrWillExpire);
   dlg.DoModal();
#endif // REGISTER

   return quit;
}

bool TrialExpired()
{
   bool quit = false;
#ifdef REGISTER
   if (theApp.fReg.IsTrial())
   {  // display trial window
      CUnregisteredVersionDlg dlg2(theApp.fReg.GetName(), theApp.fReg.GetNumber());
      if (IDOK == dlg2.DoModal())
      {  // user accepted trial or registered
         theApp.SetRegistration(dlg2.fReg.GetName(), dlg2.fReg.GetNumber());
         UINT reason = 0;
         if (!theApp.fReg.IsValid(reason))
            quit = true;   // trial version has expired
      }
      else
         quit = true;   // user did not agree to trial terms
   }
#endif // REGISTER

   return quit;
}
