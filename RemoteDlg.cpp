// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "RemoteDlg.h"
#include "AddSlaveDlg.h"
#include "Msw.h"
#include "MainFrm.h"

ARemoteDlg::ARemoteDlg() : 
   CDialog(ARemoteDlg::IDD)
      , actionButtonsBusy(false)
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
   ON_MESSAGE(WMA_UPDATE_STATUS, &OnUpdateStatus)
   ON_MESSAGE(WMA_UPDATE_CONTROLS, &OnUpdateStatus2)
END_MESSAGE_MAP()

BOOL ARemoteDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   AComm::Slaves& slaves = gComm.GetSlaves();
   for (AComm::Slaves::iterator i = slaves.begin(); i != slaves.end(); ++i)
      fSlaves.AddString(i->fUsername);

   int sel = fSlaves.FindString(-1, gComm.GetSessionJid());
   if (sel == LB_ERR)
      sel = 0;
   fSlaves.SetCurSel(sel);

   this->EnableControls();

   // set ourselves up to receive notification messages
   this->oldTarget = gComm.GetParent();
   gComm.SetParent( this->m_hWnd );

   // populate the network message box with the latest update
   gComm.ResendLastStatus(); // get actual parameter(s) last sent - statusCode and operationCode

   if (!gComm.IsMaster()) {

      // SECONDARY BOX IS SIMPLER
      // change the title
      this->SetWindowTextW(_T("Remote Operations"));

      // hide some controls
      UINT xxx = SW_HIDE;
      this->GetDlgItem(ID_REMOTE_GROUP)->ShowWindow(xxx);
      this->GetDlgItem(rCtlSlaves)->ShowWindow(xxx);
      this->GetDlgItem(ID_ADD)->ShowWindow(xxx);
      this->GetDlgItem(ID_EDIT)->ShowWindow(xxx);
      this->GetDlgItem(ID_SETTINGS)->ShowWindow(xxx);
      this->GetDlgItem(ID_REMOVE)->ShowWindow(xxx);
      this->GetDlgItem(ID_CONNECT)->ShowWindow(xxx);

      // move some other controls up into that space
      // see here for some possibly-relevant info: https://jeffpar.github.io/kbarchive/kb/145/Q145994/
      //CRect converter(0, 0, 4, 8);
      //this->MapDialogRect(converter);
      //int baseUnitY = converter.bottom;
      //double baseRatio = baseUnitY / 8.0;

      CRect posListBox, posNetMsg, posClose/*, posListBox2, posNetMsg2, posClose2*/;
      this->GetDlgItem(rCtlSlaves)->GetWindowRect(posListBox); // in dialog units or pixels?
      //posListBox2 = posListBox;
      //this->MapDialogRect(posListBox2);
      this->GetDlgItem(ID_NETMESSAGE)->GetWindowRect(posNetMsg);
      //posNetMsg2 = posNetMsg;
      //this->MapDialogRect(posNetMsg2);
      this->GetDlgItem(IDOK)->GetWindowRect(posClose);
      //posClose2 = posClose;
      //this->MapDialogRect(posClose2);
      CSize delta = posNetMsg.TopLeft() - posListBox.TopLeft();
      CPoint newClose = posClose.TopLeft() - delta;
      int fDelta = delta.cy / 2; //baseRatio; //2; // this is probably doing a rough conversion to dialog units
      this->GetDlgItem(ID_NETMESSAGE)->SetWindowPos(NULL, posListBox.left, posListBox.top - fDelta, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
      this->GetDlgItem(IDOK)->SetWindowPos(NULL, newClose.x, newClose.y - fDelta, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

      // shrink the dialog box iteself accordingly
      CRect posDlg;
      this->GetClientRect(posDlg);
      this->SetWindowPos(NULL, 0, 0, posDlg.Width(), posDlg.Height() - fDelta, SWP_NOMOVE | SWP_NOZORDER);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void ARemoteDlg::OnConnect()
{
   const int sel = fSlaves.GetCurSel();
   if (LB_ERR == sel)
      return;

   // disable the button until operation completes with status
   this->GetDlgItem(ID_CONNECT)->EnableWindow(FALSE);
   actionButtonsBusy = true;

   if (this->SelIsConnected() && gComm.IsContacted())
   {  // disconnect
      this->opInProgress = AComm::kDisconnect;
      gComm.EndChat();
   }
   else
   {  // connect (must contact through network)
      CString slave;
      fSlaves.GetText(sel, slave);
      CWaitCursor wait;
      this->opInProgress = AComm::kConnect; //kContact; // use this until the Test command is automated into the Connect call
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

LRESULT ARemoteDlg::OnUpdateStatus(WPARAM wp, LPARAM lp)
{
   CString msg, msg2 = gComm.GetLastMessage();
   AComm::OpType opCode = (AComm::OpType)wp;
   AComm::Status opStatus = (AComm::Status)lp;
   msg.Format(_T("Most recent network status: %u on op %u\n%s\n"), opStatus, opCode, (LPCTSTR)msg2);
   //TRACE(msg);
   int n = msg.Replace(_T("\n"), _T("\r\n")); // the UI edit control requires this substitution
   this->GetDlgItem(ID_NETMESSAGE)->SetWindowTextW(msg);

   // finish the busy transaction when the expected op code arrives with a result (save the res?)
   if (opCode == this->opInProgress && opStatus != AComm::kWaiting) {
      actionButtonsBusy = false;
   }
   TRACE("Received OP=%u STATUS=%u UI=%s MSG=%d lines\n", opCode, opStatus, (actionButtonsBusy ? "BUSY" : "IDLE"), n);
   ::PostMessageA(this->m_hWnd, WMA_UPDATE_CONTROLS, wp, lp);
   return 0;
}

LRESULT ARemoteDlg::OnUpdateStatus2(WPARAM, LPARAM)
{
   // kludge: delay the state checks until after op compmletes
   this->EnableControls();
   return 0;
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
   // also Login and Connect depend on state now
   const int selectedSlave = fSlaves.GetCurSel();
   BOOL isAnySlaveSel = LB_ERR != selectedSlave;
   BOOL fLogin = TRUE;
   BOOL fConnect = isAnySlaveSel;
   BOOL fEdit = isAnySlaveSel;
   BOOL fRemove = isAnySlaveSel;

   if(!gComm.IsConnected()) {
      fConnect = FALSE;
      TRACE("Tried to shut off Connect button.\n");
   }

   if (actionButtonsBusy) {
      fLogin = fConnect = FALSE;
      TRACE("Action buttons are busy.\n");
   }

   this->GetDlgItem(ID_LOGIN)->EnableWindow(fLogin);
   this->GetDlgItem(ID_CONNECT)->EnableWindow(fConnect);
   this->GetDlgItem(ID_EDIT)->EnableWindow(fEdit);
   this->GetDlgItem(ID_REMOVE)->EnableWindow(fRemove);
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

   // restore old target of notification messages
   gComm.SetParent( this->oldTarget );

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
   this->GetDlgItem(ID_LOGIN)->EnableWindow(FALSE);
   actionButtonsBusy = true;
   if (gComm.IsConnected()) {
      this->opInProgress = AComm::kLogout;
      gComm.Disconnect();
   } else {
      this->opInProgress = AComm::kLogin;
      gComm.Connect(fUsername, fPassword);
   }
}

LRESULT ARemoteDlg::OnKickIdle(WPARAM, LPARAM)
{
   CString caption;
   VERIFY(caption.LoadString(gComm.IsConnected() ? rStrBtnLogout : rStrBtnLogin));
   this->GetDlgItem(ID_LOGIN)->SetWindowText(caption);
   this->GetDlgItem(ID_LOGIN)->UpdateWindow();

   VERIFY(caption.LoadString( (this->SelIsConnected() && gComm.IsContacted())  ? rStrDisconnect : rStrConnect));
   this->GetDlgItem(ID_CONNECT)->SetWindowText(caption);
   this->GetDlgItem(ID_CONNECT)->UpdateWindow();

   return 0;
}

