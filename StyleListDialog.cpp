// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "StyleListDialog.h"
#include "StyleDialog.h"
#include "Msw.h"


IMPLEMENT_DYNAMIC(CStyleListDialog, CDialog)

CStyleListDialog::CStyleListDialog(CWnd* pParent /*=NULL*/) :
   CDialog(CStyleListDialog::IDD, pParent)
{
}

void CStyleListDialog::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_LIST1, m_lbStyles);
}


BEGIN_MESSAGE_MAP(CStyleListDialog, CDialog)
   ON_BN_CLICKED(IDC_BUTTON2, OnAddStyle)
   ON_BN_CLICKED(IDC_BUTTON3, OnEditStyle)
   ON_BN_CLICKED(IDC_BUTTON4, OnDeleteStyle)
   ON_LBN_DBLCLK(IDC_LIST1, OnEditStyle)
   ON_LBN_SELCHANGE(IDC_LIST1, &CStyleListDialog::OnSelectionChange)
END_MESSAGE_MAP()


BOOL CStyleListDialog::OnInitDialog()
{
   CDialog::OnInitDialog();

   this->UpdateList();
   m_lbStyles.SetCurSel(0);
   this->EnableButtons();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CStyleListDialog::UpdateList()
{
   m_lbStyles.ResetContent();
   for (size_t i = 0; i < gStyles.size(); ++i)
   {
   	AStyle style = gStyles[i];
		m_lbStyles.AddString(style.fName);
   }

   AfxGetMainWnd()->SendMessage(WMA_UPDATE_STYLES);
}

void CStyleListDialog::SelectByName(LPCTSTR szName)
{
   for (int i = 0; i < m_lbStyles.GetCount(); ++i)
   {
      CString stListItem;
      m_lbStyles.GetText(i, stListItem);
      if (0 == stListItem.CompareNoCase(szName))
      {
         m_lbStyles.SetCurSel(i);
         this->EnableButtons();
         break;
      }
   }
}

void CStyleListDialog::OnAddStyle()
{
   CStyleDialog dlg;
   if (IDOK == dlg.DoModal())
   {
      gStyles.push_back(dlg.fStyle);
      this->UpdateList();
      this->SelectByName(dlg.fStyle.fName);
   }
}

void CStyleListDialog::OnEditStyle()
{
   const int nCurSel = m_lbStyles.GetCurSel();
   ASSERT((nCurSel >= 0) && (nCurSel < m_lbStyles.GetCount()));

   CString name;
   m_lbStyles.GetText(nCurSel, name);
   AStyle* style = gStyles.GetStyle(name);
   ASSERT(NULL != style);

   CStyleDialog dlg(style);
   dlg.m_nIndex = gStyles.GetIndex(name);
   if (IDOK == dlg.DoModal())
   {
      *style = dlg.fStyle;
      this->UpdateList();
      this->SelectByName(style->fName);
   }
}

void CStyleListDialog::OnDeleteStyle()
{
   int nCurSel = m_lbStyles.GetCurSel();
   ASSERT((nCurSel >= 0) && (nCurSel < m_lbStyles.GetCount()));

   CString name;
   m_lbStyles.GetText(nCurSel, name);
   AStyle* style = gStyles.GetStyle(name);
   ASSERT(NULL != style);

   CString stMessage;
   stMessage.FormatMessage(IDP_CONFIRM_DELETE, style->fName);
   if (IDYES == AfxMessageBox(stMessage, MB_YESNO | MB_ICONQUESTION, IDP_CONFIRM_DELETE))
   {
      if (gStyles.Remove(style->fName))
      {
         m_lbStyles.DeleteString(nCurSel);
         if (nCurSel >= m_lbStyles.GetCount())
            nCurSel--;

         if (nCurSel >= 0)
            m_lbStyles.SetCurSel(nCurSel);

         this->EnableButtons();

         this->UpdateList();
      }
   }
}

void CStyleListDialog::EnableButtons()
{
   const bool selection = (m_lbStyles.GetCurSel() >= 0);
   this->GetDlgItem(IDC_BUTTON3)->EnableWindow(selection);
   this->GetDlgItem(IDC_BUTTON4)->EnableWindow(selection);
}
