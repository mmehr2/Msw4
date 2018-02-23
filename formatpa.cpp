// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "formatpa.h"

static const BYTE kNone = 127;   // invalid value for alignment and spacing

AFormatParaDlg::AFormatParaDlg(const CParaFormat& pf, CWnd* pParent /*=NULL*/) :
   CDialog(AFormatParaDlg::IDD, pParent),
   m_nAlignment(-1),
   m_nSpacing(-1)
{
   m_pf = pf;
   if (m_pf.dwMask & PFM_ALIGNMENT)
      m_nAlignment = this->GetAlignmentIndex(pf.wAlignment);
   if (m_pf.dwMask & PFM_SPACEAFTER)
      m_nSpacing = this->GetSpacingIndex(m_pf.bLineSpacingRule);
}

void AFormatParaDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(AFormatParaDlg)
   DDX_CBIndex(pDX, IDC_COMBO_ALIGNMENT, m_nAlignment);
   DDX_CBIndex(pDX, IDC_COMBO_SPACING, m_nSpacing);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(AFormatParaDlg, CDialog)
   //{{AFX_MSG_MAP(AFormatParaDlg)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


void AFormatParaDlg::OnOK()
{
   CDialog::OnOK();

   m_pf.dwMask = 0;
   if ((m_pf.wAlignment = this->GetAlignment(m_nAlignment)) != kNone)
      m_pf.dwMask |= PFM_ALIGNMENT;

   if ((m_pf.bLineSpacingRule = this->GetSpacing(m_nSpacing)) != kNone)
   {
      m_pf.dwMask |= PFM_LINESPACING | PFM_SPACEAFTER;
      switch (m_pf.bLineSpacingRule) {
         case 0:  m_pf.dyLineSpacing = 20;   break;
         case 1:  m_pf.dyLineSpacing = 30; break;
         case 2:  m_pf.dyLineSpacing = 40;   break;
      }
      m_pf.bLineSpacingRule = 5;
   }
}

BOOL AFormatParaDlg::OnInitDialog()
{
   // initialize alignment combo
   const int aligns[] = {IDS_LEFT, IDS_RIGHT, IDS_CENTER/*, IDS_JUSTIFY*/};
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_COMBO_ALIGNMENT);
   for (int i = 0; i < mCountOf(aligns); ++i)
   {
      CString str;
      VERIFY(str.LoadString(aligns[i]));
      pBox->AddString(str);
   }

   // initialize spacing combo
   const int spacing[] = {rStrSingleSpace, rStrOneAndAHalfSpace, rStrDoubleSpace};
   pBox = (CComboBox*)GetDlgItem(IDC_COMBO_SPACING);
   for (int i = 0; i < mCountOf(spacing); ++i)
   {
      CString str;
      VERIFY(str.LoadString(spacing[i]));
      pBox->AddString(str);
   }

   CDialog::OnInitDialog();
   return TRUE;  // return TRUE unless you set the focus to a control
              // EXCEPTION: OCX Property Pages should return FALSE
}

WORD AFormatParaDlg::GetAlignment(int index) const
{
   switch (index)
   {
   case 0:  return PFA_LEFT;
   case 1:  return PFA_RIGHT;
   case 2:  return PFA_CENTER;
   default: return kNone;
   }
}

int AFormatParaDlg::GetAlignmentIndex(int align) const
{
   switch (align)
   {
   case PFA_LEFT:    return 0;
   case PFA_RIGHT:   return 1;
   case PFA_CENTER:  return 2;
   default: return kNone;
   }
}

BYTE AFormatParaDlg::GetSpacing(int index) const
{
   switch (index)
   {
   case 0:  return 0;   // single space
   case 1:  return 1;   // one-and-a-half space
   case 2:  return 2;   // double space
   default: return kNone;
   }
}

int AFormatParaDlg::GetSpacingIndex(int spacing) const
{
   switch (spacing)
   {
   case 0:  return 0;   // single space
   case 1:  return 1;   // one-and-a-half space
   case 2:  return 2;   // double space
   default: return kNone;
   }
}
