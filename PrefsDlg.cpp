// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "PrefsDlg.h"
#include "Msw.h"


IMPLEMENT_DYNAMIC(APrefsDlg, CPropertySheet)

APrefsDlg::APrefsDlg(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
   this->AddPage(&fGeneralPage);
   this->AddPage(&fScrollPage);
   this->AddPage(&fVideoPage);

   // remove the Apply button
   m_psh.dwFlags |= PSH_NOAPPLYNOW;
}

APrefsDlg::APrefsDlg(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

BEGIN_MESSAGE_MAP(APrefsDlg, CPropertySheet)
   ON_BN_CLICKED(IDHELP, OnHelp)
   ON_WM_HELPINFO()
END_MESSAGE_MAP()

void APrefsDlg::OnHelp()
{
   theApp.ShowHelp(HH_HELP_CONTEXT, rDlgGeneralPrefs);
}

BOOL APrefsDlg::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();
   this->ModifyStyleEx(0, WS_EX_CONTEXTHELP);
   //UnitTestSerial();
   return bResult;
}

BOOL APrefsDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
   if (HELPINFO_WINDOW == pHelpInfo->iContextType)
   {
      const DWORD ids[] = {
         rCtlPrefClearPc,           rCtlPrefClearPc, 
         rCtlPrefResetTimer,        rCtlPrefResetTimer,
         rCtlPrefPrintCue,          rCtlPrefPrintCue,
         rCtlPrefInverseEditMode,   rCtlPrefInverseEditMode,
         rCtlErtCps,                rCtlErtCps,
         rCtlErtDefault,            rCtlErtDefault,
         rCtlButtonDefaultFont,     rCtlButtonDefaultFont,
         0}; 
      theApp.ShowWhatsThisHelp(ids, pHelpInfo->hItemHandle);
      return TRUE;
   }

   return CPropertySheet::OnHelpInfo(pHelpInfo);
}

#ifdef _REMOTE
#include <sstream>
#include <string>
#include <vector>

namespace {
   // Format: single string, series of strings separated by commas, each one contains a format type char, name, '=', and a value
   class PrefSerial {
      std::vector<std::string> items;
   public:
      PrefSerial() { }

      void addInt(const char* name, UINT i);
      void addString(const char* name, const char *s);

      std::string Dump() const;

      static const char DELIM_IN;
      static const char DELIM_OUT;
   };

   const char PrefSerial::DELIM_IN = '='; // field separator
   const char PrefSerial::DELIM_OUT = ','; // record separator

   void PrefSerial::addInt( const char* name, UINT i )
   {
      std::ostringstream os;
      //os << 'i';
      os << name;
      os << DELIM_IN;
      os << i;
      items.push_back( os.str() );
   }

   void PrefSerial::addString( const char* name, const char* s )
   {
      std::ostringstream os;
      //os << 's';
      os << name;
      os << DELIM_IN;
      os << s;
      items.push_back( os.str() );
   }

   std::string PrefSerial::Dump() const
   {
      std::ostringstream os;
      std::vector<std::string>::const_iterator it;
      for ( it = items.begin(); it!=items.end(); ++it) {
         if (it != items.begin())
            os << DELIM_OUT;
         os << *it;
      }
      return os.str();
   }
}

CStringA APrefsDlg::Serialize(void) const
{
   CStringA result;
   PrefSerial ps;

   ps.addInt("GPcpc", fGeneralPage.fClearPaperclips ? 1 : 0);
   ps.addInt("GPrt", fGeneralPage.fResetTimer ? 1 : 0);
   ps.addInt("GPmt", fGeneralPage.fManualTimer ? 1 : 0);
   ps.addInt("GPcps", static_cast<int>(fGeneralPage.fCharsPerSecond * 10.0));
   ps.addInt("GPiem", fGeneralPage.fInverseEditMode ? 1 : 0);
   CT2A ascii(fGeneralPage.fFontLanguage);
   ps.addString("GPfl", ascii.m_psz);

   ps.addInt("SPlb", fScrollPage.fLButton);
   ps.addInt("SPrb", fScrollPage.fRButton);
   ps.addInt("SPrsc", fScrollPage.fReverseSpeedControl ? 1 : 0);

   ps.addInt("VPc", fVideoPage.fColor ? 1 : 0);
   ps.addInt("VPiv", fVideoPage.fInverseVideo ? 1 : 0);
   ps.addInt("VPsd", fVideoPage.fScriptDividers ? 1 : 0);
   ps.addInt("VPss", fVideoPage.fShowSpeed ? 1 : 0);
   ps.addInt("VPssp", fVideoPage.fShowScriptPos ? 1 : 0);
   ps.addInt("VPst", fVideoPage.fShowTimer ? 1 : 0);
   ps.addInt("VPlo", fVideoPage.fLoop ? 1 : 0);
   ps.addInt("VPli", fVideoPage.fLink ? 1 : 0);
   ps.addInt("VPrtl", fVideoPage.fRightToLeft ? 1 : 0);

   result = ps.Dump().c_str();
   return result;
}

void APrefsDlg::Deserialize(const CStringA& input)
{
   //
   //CStringA ntoken, vtoken;
   //char ctype;
   //int pos = 0;
   //ntoken = input.Tokenize(PrefSerial::DELIM_IN.c_str(), pos);
   //if (pos == -1) return;
   //vtoken = input.Tokenize(PrefSerial::DELIM_OUT.c_str(), pos);
   std::string inp(input);
   std::istringstream is(inp);
   char /*ctype,*/ inc, outc;
   const int MAX_SIZE_TOKEN = 80; // fontLanguage string - has no = or , chars and is <80 long? and NO UNICODE
   char name[MAX_SIZE_TOKEN], value[MAX_SIZE_TOKEN];
   while (is) {
      //is >> ctype;   if (!is) return;
      is.get(name, MAX_SIZE_TOKEN, PrefSerial::DELIM_IN);   if (!is) return;
      is >> inc;   if (!is) return;
      is.get(value, MAX_SIZE_TOKEN, PrefSerial::DELIM_OUT);   if (!is) return;
      is >> outc;   if (!is) return;
      if (inc != PrefSerial::DELIM_IN) return;
      if (outc != PrefSerial::DELIM_OUT) return;
      std::string sname(name);
      int val = atoi(value);

      if (sname == "GPcpc") { fGeneralPage.fClearPaperclips = (val ? 1 : 0); continue; }
      if (sname == "GPrt") { fGeneralPage.fResetTimer = (val ? 1 : 0); continue; }
      if (sname == "GPmt") { fGeneralPage.fManualTimer = (val ? 1 : 0); continue; }
      if (sname == "GPcps") { fGeneralPage.fCharsPerSecond = (val / 10.0); continue; }
      if (sname == "GPiem") { fGeneralPage.fInverseEditMode = (val ? 1 : 0); continue; }
      if (sname == "GPfl") { fGeneralPage.fFontLanguage = (value); continue; }

      if (sname == "SPlb") { fScrollPage.fLButton = (val); continue; }
      if (sname == "SPrb") { fScrollPage.fRButton = (val); continue; }
      if (sname == "SPrsc") { fScrollPage.fReverseSpeedControl = (val ? 1 : 0); continue; }

      if (sname == "VPc") { fVideoPage.fColor = (val ? 1 : 0); continue; }
      if (sname == "VPiv") { fVideoPage.fInverseVideo = (val ? 1 : 0); continue; }
      if (sname == "VPsd") { fVideoPage.fScriptDividers = (val ? 1 : 0); continue; }
      if (sname == "VPss") { fVideoPage.fShowSpeed = (val ? 1 : 0); continue; }
      if (sname == "VPssp") { fVideoPage.fShowScriptPos = (val ? 1 : 0); continue; }
      if (sname == "VPst") { fVideoPage.fShowTimer = (val ? 1 : 0); continue; }
      if (sname == "VPlo") { fVideoPage.fLoop = (val ? 1 : 0); continue; }
      if (sname == "VPli") { fVideoPage.fLink = (val ? 1 : 0); continue; }
      if (sname == "VPrtl") { fVideoPage.fRightToLeft = (val ? 1 : 0); continue; }
   }
}

bool APrefsDlg::UnitTestSerial() const
{
   CStringA s1, s2;
   s1 = this->Serialize();
   APrefsDlg dlg(_T("Unit Test Data"));
   dlg.Deserialize(s1);
   s2 = dlg.Serialize();
   bool result = (s1 == s2);
   TRACE("Settings Test %s\n%s\n%s\n", result? "PASSED" : "FAILED", (LPCSTR)s1, (LPCSTR)s2);
   return result;
}
#endif // _REMOTE
