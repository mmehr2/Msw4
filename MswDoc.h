// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


#include "Msw.h"

class AFormatBar;
class AMswSrvrItem;
class AMswView;

class AMswDoc : public CRichEditDoc
{
protected: // create from serialization only
	AMswDoc();
	DECLARE_DYNCREATE(AMswDoc)

public:  // methods
	void SetDocType(DocType::Id nDocType, BOOL bNoOptionChange = FALSE);
	AMswView* GetView();
	CLSID GetClassID();
	LPCTSTR GetSection();

	virtual CFile* GetFile(LPCTSTR pszPathName, UINT nOpenFlags,
		CFileException* pException);
	virtual BOOL DoSave(LPCTSTR pszPathName, BOOL bReplace = TRUE);
	int MapType(int nType);
	void ForceDelayed(CMDIFrameWnd* pFrameWnd);
   int GetStreamFormat() const;

   virtual CRichEditCntrItem* CreateClientItem(REOBJECT*) const;
	virtual void OnDeactivateUI(BOOL bUndoable);
	virtual void Serialize(CArchive& ar);
	//{{AFX_VIRTUAL(AMswDoc)

   virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
	//}}AFX_VIRTUAL

public:  // data
   DocType::Id m_nDocType;
	DocType::Id m_nNewDocType;

protected:
	//{{AFX_MSG(AMswDoc)
	afx_msg void OnFileSendMail();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	static HICON sIconDoc;

   afx_msg void OnFileSaveCopyAs();
};
