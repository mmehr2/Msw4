// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"

#include "UnregisteredVersionDlg.h"
#include "RegisterDlg.h"
#include "Strings.h"


IMPLEMENT_DYNAMIC(CUnregisteredVersionDlg, CDialog)
CUnregisteredVersionDlg::CUnregisteredVersionDlg(LPCTSTR name, LPCTSTR number, CWnd* pParent /*=NULL*/)
   : CDialog(CUnregisteredVersionDlg::IDD, pParent)
{
   fComputerId = theApp.fReg.GetComputerId();
   fReg.SetInfo(name, number);
   fEvaluationDays = 0;
   fCancel = false;  // Set to true if Cancel should replace Quit on Cancel button
}

void CUnregisteredVersionDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_BUTTON2, fBtnBuyNow);
   DDX_Text(pDX, IDC_COMPUTER_ID, fComputerId);
}


BEGIN_MESSAGE_MAP(CUnregisteredVersionDlg, CDialog)
   ON_BN_CLICKED(IDC_BUTTON1, OnRegister)
   ON_BN_CLICKED(IDC_BUTTON2, OnBuyNow)
   ON_WM_DRAWITEM()
END_MESSAGE_MAP()


void CUnregisteredVersionDlg::OnRegister()
{
   ARegisterDlg dlg(fReg.GetName(), fReg.GetNumber());
   if (IDOK == dlg.DoModal())
   {
      fReg.SetInfo(dlg.fReg.GetName(), dlg.fReg.GetNumber());
      this->EndDialog(IDOK);
   }
}

BOOL CUnregisteredVersionDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // Make some static vars bold and slightly bigger
   int boldText[] = {IDC_TEXT2, IDC_TEXT3, IDC_TEXT4};
   for (int i = 0; i < _countof(boldText); ++i)
   {
      CWnd* pWnd = this->GetDlgItem(boldText[i]);
      ASSERT(NULL != pWnd);
      pWnd->ModifyStyle(0, SS_OWNERDRAW);
      if (NULL == fFontBold.GetSafeHandle())
      {
         LOGFONT lf;
         this->GetFont()->GetLogFont(&lf);
         lf.lfWeight = FW_BOLD;
         fFontBold.CreateFontIndirect(&lf);
         lf.lfHeight = (lf.lfHeight * 5) / 4;
         fFontThankYou.CreateFontIndirect(&lf);
      }
      pWnd->SetFont(&fFontBold);
   }

   CString message;
   if (fCancel)
   {
      CWnd* pWnd = this->GetDlgItem(IDCANCEL);
      if (NULL != pWnd)
      {
         VERIFY(message.LoadString(rStrCancelBtn));
         pWnd->SetWindowText(message);
      }
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CUnregisteredVersionDlg::OnBuyNow()
{
   ::ShellExecute(NULL, _T("open"), gStrBuyUrl, NULL, NULL, SW_SHOWNORMAL);
}

void CUnregisteredVersionDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   if (lpDrawItemStruct->CtlType == ODT_STATIC)
   {
      CString text;

      CWnd window;
      VERIFY(window.Attach(lpDrawItemStruct->hwndItem));
      window.GetWindowText(text);
      CDC dc;
      VERIFY(dc.Attach(lpDrawItemStruct->hDC));
      dc.SaveDC();
      dc.SetBkMode(TRANSPARENT);
      UINT uStyle = (IDC_TEXT2 == nIDCtl) ? DT_CENTER : DT_LEFT;
      dc.SelectObject((IDC_TEXT2 == nIDCtl) ? &fFontThankYou : &fFontBold);
      dc.DrawText(text, &lpDrawItemStruct->rcItem, uStyle);
      dc.RestoreDC(-1);
      dc.Detach();

      window.Detach();
   }

   CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}
