#pragma once

#include <afxwin.h>
#include "resource.h"

class CMemHogDlg : public CDialog
{
public:
	CMemHogDlg(CWnd* pParent = NULL);	// standard constructor

	enum { IDD = IDD_MEMHOG_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

protected:
	virtual BOOL OnInitDialog();
   afx_msg void OnAllocate();
   afx_msg void OnTimer(UINT_PTR nIDEvent);
   void Update();
	DECLARE_MESSAGE_MAP()

private:
   DWORD fAllocate;
   CString fStatus;
};
