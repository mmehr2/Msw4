// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "splash.h"
#include "Msw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG
#  include "MemHogDlg.h"
   CMemHogDlg gMemHogDlg;
#endif // _DEBUG

static CString GetMswVersion();

ASplashWnd::ASplashWnd(bool timeout /*= false*/) :
   CDialog(ASplashWnd::IDD),
   fTimeout(timeout)
{
#ifdef REGISTER
   fComputerId = theApp.fReg.GetComputerId();
#endif // REGISTER
	//{{AFX_DATA_INIT(ASplashWnd)
	//}}AFX_DATA_INIT
}

void ASplashWnd::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(ASplashWnd)
   DDX_Control(pDX, rCtlCompany, fCompanyLogo);
   DDX_Text(pDX, rCtlVersion, fVersion);
#ifdef REGISTER
   DDX_Text(pDX, IDC_COMPUTER_ID, fComputerId);
#endif // REGISTER
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(ASplashWnd, CDialog)
	//{{AFX_MSG_MAP(ASplashWnd)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
   ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL ASplashWnd::OnInitDialog()
{
   fVersion = ::GetMswVersion();

   CDialog::OnInitDialog();
	this->CenterWindow();

#ifdef MS_DEMO
   int bitmap = rBmpCompanyLogo1;
#else
   int bitmap = rBmpCompanyLogo;
#endif

   fBitmap.Attach(::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bitmap),
    IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
   fCompanyLogo.SetBitmap(fBitmap);

   if (fTimeout)
      fTimer = this->SetTimer(kTimerId, kTimerDuration, NULL);

#ifdef _DEBUG
   if (::GetKeyState(VK_SHIFT) < 0)
   {
      if (NULL == gMemHogDlg.m_hWnd)
         gMemHogDlg.Create(CMemHogDlg::IDD);
      gMemHogDlg.ShowWindow(SW_SHOW);
   }
#endif // _DEBUG

   return TRUE;  // return TRUE  unless you set the focus to a control
}

// tell linker to link with version.lib for VerQueryValue, etc.
#pragma comment(linker, "/defaultlib:version.lib")
CString GetMswVersion()
{
   CString result;

	// get module file name
   TCHAR filename[_MAX_PATH] = {0};
   DWORD len = ::GetModuleFileName(NULL, filename, mCountOf(filename));
	if (len > 0)
   {  // read file ver info
	   DWORD dwDummyHandle; // will always be set to zero
      len = ::GetFileVersionInfoSize(filename, &dwDummyHandle);
	   if (len > 0)
      {
	      BYTE* pVersionInfo = new BYTE[len]; // allocate version info
	      if (::GetFileVersionInfo(filename, 0, len, pVersionInfo))
         {
	         LPVOID lpvi;
	         UINT iLen;
	         if (VerQueryValue(pVersionInfo, _T("\\"), &lpvi, &iLen))
            {  // copy fixed info
               VS_FIXEDFILEINFO i = *(VS_FIXEDFILEINFO*)lpvi;
#ifndef _EXPIRE
               result.Format(_T("MagicScroll Version %i.%i.%i%c%i"), 
#else // _EXPIRE
               result.Format(_T("MagicScroll Evaluation: Version %i.%i.%i%c%i"), 
#endif // _EXPIRE
                  HIWORD(i.dwProductVersionMS), LOWORD(i.dwProductVersionMS), 
                  HIWORD(i.dwProductVersionLS), HIBYTE(LOWORD(i.dwProductVersionLS)),
                  LOBYTE(LOWORD(i.dwProductVersionLS)));
            }
         }
	      delete [] pVersionInfo;
      }
   }

   return result;
}

void ASplashWnd::OnTimer(UINT_PTR nIDEvent)
{
   this->KillTimer(fTimer);
   this->OnLButtonDown(0, CPoint());
   CDialog::OnTimer(nIDEvent);
}
