// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


struct CHARHDR : public tagNMHDR
{
	CHARFORMAT2 cf;
	CHARHDR() {cf.cbSize = sizeof(CHARFORMAT2);}
};

#define FN_SETFORMAT    0x1000
#define FN_GETFORMAT    0x1001

class ALocalComboBox : public CComboBox
{
public:
   ALocalComboBox();

	CPtrArray m_arrayFontDesc;
	static int m_nFontHeight;
	int m_nLimitText;

   BOOL HasFocus()
	{
		HWND hWnd = ::GetFocus();
		return (hWnd == m_hWnd || ::IsChild(m_hWnd, hWnd));
	}
	void GetTheText(CString& str);
	void SetTheText(LPCTSTR lpszText,BOOL bMatchExact = FALSE);
	BOOL LimitText(int nMaxChars);

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	//{{AFX_MSG(ALocalComboBox)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class AFontComboBox : public ALocalComboBox
{
public:
	AFontComboBox();

	CBitmap m_bmFontType;

	void EnumFontFamiliesEx(CDC& dc, BYTE nCharSet = DEFAULT_CHARSET);
	void AddFont(ENUMLOGFONT* pelf, DWORD dwType, LPCTSTR lpszScript = NULL);
	void MatchFont(LPCTSTR lpszName, BYTE nCharSet);
	void EmptyContents();
	static BOOL CALLBACK AFX_EXPORT EnumFamScreenCallBack(
		ENUMLOGFONT* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType,
		LPVOID pThis);
	static BOOL CALLBACK AFX_EXPORT EnumFamPrinterCallBack(
		ENUMLOGFONT* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType,
		LPVOID pThis);
	static BOOL CALLBACK AFX_EXPORT EnumFamScreenCallBackEx(
		ENUMLOGFONTEX* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType,
		LPVOID pThis);
	static BOOL CALLBACK AFX_EXPORT EnumFamPrinterCallBackEx(
		ENUMLOGFONTEX* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType,
		LPVOID pThis);

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT lpCIS);
	//{{AFX_MSG(AFontComboBox)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class ASizeComboBox : public ALocalComboBox
{
public:
	ASizeComboBox();

	int m_nLogVert;
	int m_nTwipsLast;

public:
	void EnumFontSizes(CDC& dc, LPCTSTR pFontName);
	static BOOL FAR PASCAL EnumSizeCallBack(LOGFONT FAR* lplf,
		LPNEWTEXTMETRIC lpntm,int FontType, LPVOID lpv);
	CString TwipsToPointString(int nTwips);
	void SetTwipSize(int nSize);
	int GetTwipSize();
	void InsertSize(int nSize);
};

class AFormatBar : public CToolBar
{
public:
	AFormatBar();

public:
	void PositionCombos();
	void FillStyleList();
	void FillFontList();
	void SyncToView();

public:
	CDC m_dcPrinter;
	CSize m_szBaseUnits;
	AFontComboBox m_comboFontName;
	ALocalComboBox	m_comboStyles;
	ASizeComboBox m_comboFontSize;

public:
	void NotifyView(UINT nCode);
	void SetStyleComboBox(LPCTSTR szStyle);
	void SetCharFormat(CCharFormat& cf);

protected:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

	//{{AFX_MSG(AFormatBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnFontNameKillFocus();
	afx_msg void OnFontSizeKillFocus();
	afx_msg void OnFontStyleKillFocus();
	afx_msg void OnFontSizeDropDown();
	afx_msg void OnComboCloseUp();
	afx_msg void OnComboSetFocus();
	afx_msg LONG OnPrinterChanged(UINT, LONG); //handles registered message
	DECLARE_MESSAGE_MAP()
};
