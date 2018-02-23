// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "RemoteDlg.h"
#include "AddSlaveDlg.h"
#include "Msw.h"
#include "MainFrm.h"

ARemoteDlg::ARemoteDlg() : 
   CDialog(ARemoteDlg::IDD)
{
   fUsername = gComm.GetUsername();
   fPassword = gComm.GetPassword();
}

void ARemoteDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, rCtlMe, fUsername);
   DDX_Text(pDX, rCtlMe2, fPassword);
   DDX_Control(pDX, rCtlSlaves, fSlaves);
}

BEGIN_MESSAGE_MAP(ARemoteDlg, CDialog)
   ON_LBN_DBLCLK(rCtlSlaves, &OnConnect)
   ON_BN_CLICKED(ID_CONNECT, &OnConnect)
   ON_BN_CLICKED(ID_ADD, &OnAdd)
   ON_BN_CLICKED(ID_EDIT, &OnEdit)
   ON_BN_CLICKED(ID_SETTINGS, &OnSettings)
   ON_BN_CLICKED(ID_REMOVE, &OnRemove)
   ON_LBN_SELCHANGE(rCtlSlaves, &ARemoteDlg::OnSlaveChange)
   ON_BN_CLICKED(ID_LOGIN, &ARemoteDlg::OnLogin)
   ON_MESSAGE(WM_KICKIDLE, &OnKickIdle)
END_MESSAGE_MAP()

BOOL ARemoteDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   AComm::Slaves& slaves = gComm.GetSlaves();
   for (AComm::Slaves::iterator i = slaves.begin(); i != slaves.end(); ++i)
      fSlaves.AddString(i->fUsername);

   fSlaves.SetCurSel(0);

   this->EnableControls();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void ARemoteDlg::OnConnect()
{
   const int sel = fSlaves.GetCurSel();
   if (LB_ERR == sel)
      return;

   if (this->SelIsConnected())
   {  // disconnect
      gComm.EndChat();
   }
   else
   {  // connect
      CString slave;
      fSlaves.GetText(sel, slave);
      CWaitCursor wait;
      gComm.StartChat(slave);
   }
}

void ARemoteDlg::OnAdd()
{
   AAddSlaveDlg dlg;
   if (IDOK == dlg.DoModal())
   {
      fSlaves.AddString(dlg.fUsername);
      this->EnableControls();
   }
}

void ARemoteDlg::OnEdit()
{
   const int sel = fSlaves.GetCurSel();
   ASSERT(LB_ERR != sel);
   if (sel == fSlaves.FindString(-1, gComm.GetSessionJid()))
   {
      CString message;
      message.FormatMessage(rStrEditConnSlave, (LPCTSTR)gComm.GetSessionJid());
      ::AfxMessageBox(message);
      return;
   }

   CString slave;
   fSlaves.GetText(sel, slave);

   AAddSlaveDlg dlg;
   dlg.fUsername = slave;
   if (IDOK == dlg.DoModal())
   {
      fSlaves.DeleteString(sel);
      fSlaves.AddString(dlg.fUsername);
      this->EnableControls();
   }
}

void ARemoteDlg::OnSettings()
{
   dynamic_cast<AMainFrame*>(::AfxGetMainWnd())->OnPreferences();
}

void ARemoteDlg::OnRemove()
{
   const int sel = fSlaves.GetCurSel();
   ASSERT(LB_ERR != sel);
   if (sel == fSlaves.FindString(-1, gComm.GetSessionJid()))
   {
      CString message;
      message.FormatMessage(rStrEditConnSlave, (LPCTSTR)gComm.GetSessionJid());
      ::AfxMessageBox(message);
      return;
   }

   fSlaves.DeleteString(sel);
   this->EnableControls();
}

void ARemoteDlg::EnableControls()
{  // enable buttons based on selection of a slave
   const int selectedSlave = fSlaves.GetCurSel();
   this->GetDlgItem(ID_CONNECT)->EnableWindow(LB_ERR != selectedSlave);
   this->GetDlgItem(ID_EDIT)->EnableWindow(LB_ERR != selectedSlave);
   this->GetDlgItem(ID_REMOVE)->EnableWindow(LB_ERR != selectedSlave);
}

void ARemoteDlg::OnSlaveChange()
{
   this->EnableControls();
}

void ARemoteDlg::OnOK()
{
   AComm::Slaves& slaves = gComm.GetSlaves();
   slaves.clear();
   for (UINT i = 0, n = fSlaves.GetCount(); i < n; ++i)
   {
      AComm::ASlave slave;
      fSlaves.GetText(i, slave.fUsername);
      slaves.push_back(slave);
   }

   gComm.SetUsername(fUsername);
   gComm.SetPassword(fPassword);

   CDialog::OnOK();
}

bool ARemoteDlg::SelIsConnected() const
{
   const int conn = fSlaves.FindString(-1, gComm.GetSessionJid());
   return (LB_ERR != conn) && (fSlaves.GetCurSel() == conn);
}

void ARemoteDlg::OnLogin()
{  // or logout
   this->UpdateData();
   gComm.IsConnected()? gComm.Disconnect() : gComm.Connect(fUsername, fPassword);
}

LRESULT ARemoteDlg::OnKickIdle(WPARAM, LPARAM)
{
   CString caption;
   VERIFY(caption.LoadString(gComm.IsConnected() ? rStrBtnLogout : rStrBtnLogin));
   this->GetDlgItem(ID_LOGIN)->SetWindowText(caption);
   this->GetDlgItem(ID_LOGIN)->UpdateWindow();

   VERIFY(caption.LoadString(this->SelIsConnected() ? rStrDisconnect : rStrConnect));
   this->GetDlgItem(ID_CONNECT)->SetWindowText(caption);
   this->GetDlgItem(ID_CONNECT)->UpdateWindow();

   return 0;
}