// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "captionspg.h"
#include "caption.h"

#include <vector>

ACaptionsPage::ACaptionsPage() : CPropertyPage(ACaptionsPage::IDD)
{
	//{{AFX_DATA_INIT(ACaptionsPage)
	//}}AFX_DATA_INIT
}


void ACaptionsPage::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);

   CString parity, stopBits, baudRate, byteSize;
   if (!pDX->m_bSaveAndValidate) {
      parity.Format(_T("%s"), (fParity==NOPARITY)? _T("None") : (fParity==ODDPARITY)? _T("Odd") : (fParity==EVENPARITY)? _T("Even") : (fParity==MARKPARITY)? _T("Mark") : _T("Pace"));
      stopBits.Format(_T("%s"), (fStopBits==ONESTOPBIT)? _T("1") : (fStopBits==ONE5STOPBITS)? _T("1.5") : _T("2"));
      byteSize.Format(_T("%d"), fByteSize);
      baudRate.Format(_T("%d"), fBaudRate);
      DDX_CBString(pDX, rCtlParity, parity);
      DDX_CBString(pDX, rCtlStopBits, stopBits);
   }

   //{{AFX_DATA_MAP(ACaptionsPage)
   DDX_Check(pDX, rCtlCcEnable, fEnabled);
   DDX_Control(pDX, rCtlPort, fPortList);
   DDX_CBString(pDX, rCtlPort, fPort);
   DDX_CBString(pDX, rCtlBaudRate, baudRate);
   DDX_CBString(pDX, rCtlByteSize, byteSize);
   //}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate) {
      DDX_CBIndex(pDX, rCtlParity, fParity);
      DDX_CBIndex(pDX, rCtlStopBits, fStopBits);
      ::_stscanf_s(baudRate, _T("%d"), &fBaudRate);
      ::_stscanf_s(byteSize, _T("%d"), &fByteSize);
   }
}

BEGIN_MESSAGE_MAP(ACaptionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(ACaptionsPage)
   ON_WM_CLOSE()
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(rCtlCcEnable, &ACaptionsPage::OnEnable)
END_MESSAGE_MAP()

BOOL ACaptionsPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   // populate the ports control with available ports
   CWaitCursor wait;
   std::vector<CString> ports = ACaption::GetPorts();
   for (std::vector<CString>::iterator i = ports.begin(); i != ports.end(); ++i) {
      fPortList.AddString(*i);
   }
   fPortList.SelectString(-1, fPort);

   this->Enable();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void ACaptionsPage::OnEnable() {
   this->UpdateData();
   this->Enable();
}

void ACaptionsPage::Enable() {
   this->GetDlgItem(rCtlPort)->EnableWindow(fEnabled);
   this->GetDlgItem(rCtlParity)->EnableWindow(fEnabled);
   this->GetDlgItem(rCtlStopBits)->EnableWindow(fEnabled);
   this->GetDlgItem(rCtlBaudRate)->EnableWindow(fEnabled);
   this->GetDlgItem(rCtlByteSize)->EnableWindow(fEnabled);
}