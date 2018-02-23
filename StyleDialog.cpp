// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "StyleDialog.h"
#include "Msw.h"


IMPLEMENT_DYNAMIC(CStyleDialog, CDialog)
CStyleDialog::CStyleDialog(AStyle* style, CWnd* pParent /*=NULL*/) :
   CDialog(CStyleDialog::IDD, pParent),
	m_nIndex(-1)
{
   if (NULL == style)
   {  // set default
      fStyle.fFormat.dwMask = CFM_ALL2;
      fStyle.fFormat.SetFaceName(_T("Arial"));
      fStyle.fFormat.yHeight = 240;
   }
   else
      fStyle = *style;
}

void CStyleDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_EDIT1, fStyle.fName);
}

BEGIN_MESSAGE_MAP(CStyleDialog, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, OnFont)
	ON_WM_PAINT()
END_MESSAGE_MAP()

CRect CStyleDialog::GetFontRect()
{
   CRect rcFont;
	CWnd* pWnd = GetDlgItem(IDC_GROUP1);
	pWnd->GetWindowRect(rcFont);
	rcFont.DeflateRect(7,14,7,7);
	ScreenToClient(rcFont);
	return rcFont;
}

void CStyleDialog::OnPaint()
{
   CRect rcFont = this->GetFontRect();
	CPaintDC dc(this); // device context for painting
	dc.SaveDC();
   dc.FillSolidRect(rcFont, ::GetSysColor(COLOR_3DFACE));

   CFont font;
	CFontDialog dlg(fStyle.fFormat);
   dlg.FillInLogFont(fStyle.fFormat);
	if (!font.CreateFontIndirect(&dlg.m_lf))
		TRACE("  Failed to Create Font\n");
	font.GetLogFont(&dlg.m_lf);
	dc.SelectObject(&font);
	dc.SetTextColor(fStyle.fFormat.crTextColor);
	dc.DrawText(dlg.m_lf.lfFaceName, lstrlen(dlg.m_lf.lfFaceName), rcFont, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	dc.RestoreDC(-1);
}

void CStyleDialog::OnFont()
{
   // convert to the old format
   CHARFORMAT oldFormat = fStyle.fFormat;
   oldFormat.cbSize = sizeof(oldFormat);
	CFontDialog dlg(fStyle.fFormat);
	if (dlg.DoModal() == IDOK)
   {
		dlg.GetCharFormat(oldFormat);
      fStyle.fFormat = oldFormat;
   }

	CRect rcFont = GetFontRect();
   this->InvalidateRect(rcFont);
}

void CStyleDialog::OnOK()
{  // validate the name
   CString msg;
	this->UpdateData(TRUE);
	int i = gStyles.GetIndex(fStyle.fName);
   if (fStyle.fName.IsEmpty())            // an empty style name is illegal
		msg.FormatMessage(IDP_NO_STYLE_NAME);
	else if ((i >= 0) && (i != m_nIndex))  // Can't have two styles with the same name
		msg.FormatMessage(IDP_DUPLICATE_STYLE, (LPCTSTR)fStyle.fName);
   else
	   CDialog::OnOK();

   if (!msg.IsEmpty())
      ::AfxMessageBox(msg, MB_OK);
}
