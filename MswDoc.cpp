// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"

#include "childfrm.h"
#include "formatba.h"
#include "mainfrm.h"
#include "mswdoc.h"
#include "mswview.h"
#include "multconv.h"
#include "strings.h"
#include "cntritem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);

HICON AMswDoc::sIconDoc = NULL;

IMPLEMENT_DYNCREATE(AMswDoc, CRichEditDoc)

BEGIN_MESSAGE_MAP(AMswDoc, CRichEditDoc)
	//{{AFX_MSG_MAP(AMswDoc)
	ON_COMMAND(rCmdFileSend, OnFileSendMail)
	ON_UPDATE_COMMAND_UI(rCmdFileSend, OnUpdateFileSendMail)
   ON_COMMAND(rCmdFileSaveCopyAs, &AMswDoc::OnFileSaveCopyAs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

AMswDoc::AMswDoc() :
   m_nDocType(DocType::RD_UNKNOWN),
	m_nNewDocType(DocType::RD_UNKNOWN)
{
}

BOOL AMswDoc::OnNewDocument()
{
	if (CRichEditDoc::OnNewDocument())
   {
	   GetView()->SetDefaultFont();
	   SetDocType(theApp.m_nNewDocType);
	   return TRUE;
   }
   else
		return FALSE;
}

void AMswDoc::ReportSaveLoadException(LPCTSTR lpszPathName,
	CException* e, BOOL bSaving, UINT nIDP)
{
	if (m_bDeferErrors || (NULL == e) || !e->IsKindOf(RUNTIME_CLASS(CFileException)))
	   CRichEditDoc::ReportSaveLoadException(lpszPathName, e, bSaving, nIDP);
   else
	{
		ASSERT_VALID(e);
		switch (((CFileException*)e)->m_cause)
		{
		case CFileException::fileNotFound:
		case CFileException::badPath:          nIDP = AFX_IDP_FAILED_INVALID_PATH; break;
		case CFileException::diskFull:         nIDP = AFX_IDP_FAILED_DISK_FULL; break;
		case CFileException::tooManyOpenFiles: nIDP = IDS_TOOMANYFILES; break;
		case CFileException::directoryFull:    nIDP = IDS_DIRFULL; break;
		case CFileException::sharingViolation: nIDP = IDS_SHAREVIOLATION; break;
      case CFileException::lockViolation:
		case CFileException::badSeek:
		case CFileException::genericException:
		case CFileException::invalidFile:
		case CFileException::hardIO:           nIDP = bSaving ? AFX_IDP_FAILED_IO_ERROR_WRITE : AFX_IDP_FAILED_IO_ERROR_READ; break;
		default:                               break;

      case CFileException::accessDenied:
			nIDP = bSaving ? AFX_IDP_FAILED_ACCESS_WRITE : AFX_IDP_FAILED_ACCESS_READ;
			if (((CFileException*)e)->m_lOsError == ERROR_WRITE_PROTECT)
				nIDP = IDS_WRITEPROTECT;
			break;
		}
		CString prompt;
		AfxFormatString1(prompt, nIDP, lpszPathName);
		AfxMessageBox(prompt, MB_ICONEXCLAMATION, nIDP);
	}
}

BOOL AMswDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	CFileException fe;
   DocType::Id supposedType = m_nNewDocType;
	m_nNewDocType = GetDocTypeFromName(lpszPathName, supposedType, fe);
   if (DocType::RD_UNKNOWN == m_nNewDocType)
	{
		ReportSaveLoadException(lpszPathName, &fe, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
		return FALSE;
	}
	else if ((m_nNewDocType == DocType::RD_TEXT) && theApp.m_bForceOEM)
		m_nNewDocType = DocType::RD_OEMTEXT;

   ScanForConverters();
	if (!gDocTypes[m_nNewDocType].bRead)
	{
		CString str;
		CString strName = gDocTypes[m_nNewDocType].GetString(DOCTYPE_DOCTYPE);
		AfxFormatString1(str, IDS_CANT_LOAD, strName);
		AfxMessageBox(str, MB_OK|MB_ICONINFORMATION);
		return FALSE;
	}

   if ((m_nNewDocType != supposedType) && !::IsTextType(m_nNewDocType))
      ::AfxMessageBox(rStrNoConverter);

   return CRichEditDoc::OnOpenDocument(lpszPathName);
}

void AMswDoc::Serialize(CArchive& ar)
{
	COleMessageFilter* pFilter = AfxOleGetMessageFilter();
	pFilter->EnableBusyDialog(FALSE);
	if (ar.IsLoading())
		SetDocType(m_nNewDocType);

	AMswView* pView = this->GetView();
	if (pView != NULL)
		pView->Serialize(ar);
	// we don't call the base class COleServerDoc::Serialize
	// because we don't want the client items serialized
	// the client items are handled directly by the RichEdit control

   pFilter->EnableBusyDialog(TRUE);
}

BOOL AMswDoc::DoSave(LPCTSTR pszPathName, BOOL bReplace /*=TRUE*/)
{
	CString newName = pszPathName;
	DocType::Id nOrigDocType = m_nDocType;  //saved in case of SaveCopyAs or failure
	BOOL bModified = IsModified();

	ScanForConverters();

	BOOL bSaveAs = FALSE;
	if (newName.IsEmpty())
		bSaveAs = TRUE;
	else if (!gDocTypes[m_nDocType].bWrite)
	{
		if (AfxMessageBox(IDS_SAVE_UNSUPPORTED, MB_YESNO | MB_ICONQUESTION) != IDYES)
		{
			return FALSE;
		}
		else
			bSaveAs = TRUE;
	}

	if (bModified && !bSaveAs && IsTextType(m_nDocType) && !GetView()->IsFormatText())
	{  // formatting changed in plain old text file
      if (IDNO == ::AfxMessageBox(IDS_SAVE_FORMAT_TEXT, MB_YESNO))
         return FALSE;
	}

	GetView()->GetParentFrame()->RecalcLayout();
	if (bSaveAs)
	{
		newName = m_strPathName;
		if (bReplace && newName.IsEmpty())
		{
			newName = m_strTitle;
			int iBad = newName.FindOneOf(_T(" #%;/\\"));    // dubious filename
			if (iBad != -1)
				newName.ReleaseBuffer(iBad);

			// append the default suffix if there is one
			newName += GetExtFromType(m_nDocType);
		}

		DocType::Id nDocType = m_nDocType;
		if (!theApp.PromptForFileName(newName,
			bReplace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY,
			OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, &nDocType))
		{
			SetDocType(nOrigDocType, TRUE);
			return FALSE;       // don't even try to save
		}
		SetDocType(nDocType, TRUE);
	}

	BeginWaitCursor();
	if (!OnSaveDocument(newName))
	{
		if (pszPathName == NULL)
		{
			// be sure to delete the file
			TRY
			{
				CFile::Remove(newName);
			}
			CATCH_ALL(e)
			{
				TRACE0("Warning: failed to delete file after failed SaveAs\n");
			}
			END_CATCH_ALL
		}
		// restore orginal document type
		SetDocType(nOrigDocType, TRUE);
		EndWaitCursor();
		return FALSE;
	}

	EndWaitCursor();
	if (bReplace)
	{
		DocType::Id nType = m_nDocType;
		SetDocType(nOrigDocType, TRUE);
		SetDocType(nType);
		// Reset the title and change the document name
		SetPathName(newName, TRUE);
		ASSERT(m_strPathName == newName);       // must be set

      // update the script queue
      theApp.fScriptQueueDlg.UpdateNames();
	}
	else // SaveCopyAs
	{
		SetDocType(nOrigDocType, TRUE);
		SetModifiedFlag(bModified);
	}
	return TRUE;        // success
}

void AMswDoc::OnDeactivateUI(BOOL bUndoable)
{
	if (GetView()->m_bDelayUpdateItems)
		UpdateAllItems(NULL);
	CRichEditDoc::OnDeactivateUI(bUndoable);
}

void AMswDoc::ForceDelayed(CMDIFrameWnd* pFrameWnd)
{
	ASSERT_VALID(this);
	ASSERT_VALID(pFrameWnd);

	POSITION pos = pFrameWnd->m_listControlBars.GetHeadPosition();
	while (pos != NULL)
	{
		// show/hide the next control bar
		CControlBar* pBar =
			(CControlBar*)pFrameWnd->m_listControlBars.GetNext(pos);

		BOOL bVis = pBar->GetStyle() & WS_VISIBLE;
		UINT swpFlags = 0;
		if ((pBar->m_nStateFlags & CControlBar::delayHide) && bVis)
			swpFlags = SWP_HIDEWINDOW;
		else if ((pBar->m_nStateFlags & CControlBar::delayShow) && !bVis)
			swpFlags = SWP_SHOWWINDOW;
		pBar->m_nStateFlags &= ~(CControlBar::delayShow|CControlBar::delayHide);
		if (swpFlags != 0)
		{
			pBar->SetWindowPos(NULL, 0, 0, 0, 0, swpFlags|
				SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}
}

CLSID AMswDoc::GetClassID()
{
	return (m_pFactory == NULL) ? CLSID_NULL : m_pFactory->GetClassID();
}

void AMswDoc::SetDocType(DocType::Id nNewDocType, BOOL)
{
   ASSERT(nNewDocType != DocType::RD_UNKNOWN);
	if (nNewDocType == m_nDocType)
		return;

	m_bRTF = !IsTextType(nNewDocType);
   m_nDocType = nNewDocType;

   AChildFrame* frame = dynamic_cast<AChildFrame*>(this->GetView()->GetParentFrame());
   frame->SetIcon(sIconDoc, true);
}

AMswView* AMswDoc::GetView()
{
	POSITION pos = GetFirstViewPosition();
	return (AMswView* )GetNextView( pos );
}

CFile* AMswDoc::GetFile(LPCTSTR pszPathName, UINT nOpenFlags, CFileException* pException)
{
	ATrackFile* pFile = NULL;
	CMDIFrameWnd* pWnd = dynamic_cast<CMDIFrameWnd*>(GetView()->GetParentFrame());
#ifdef CONVERTERS
	ScanForConverters();

	// if writing use current doc type otherwise use new doc type
	int nType = (nOpenFlags & CFile::modeReadWrite) ? m_nDocType : m_nNewDocType;
	// m_nNewDocType will be same as m_nDocType except when opening a new file
	if ((nType >= 0) && (nType < _countof(gDocTypes)) && (gDocTypes[nType].pszConverterName != NULL))
		pFile = new AConverter(gDocTypes[nType].pszConverterName, pWnd);
	else
#endif
	if (nType == DocType::RD_OEMTEXT)
		pFile = new AOemFile(pWnd);
	else
		pFile = new ATrackFile(pWnd);
	if (!pFile->Open(pszPathName, nOpenFlags, pException))
	{
		delete pFile;
		return NULL;
	}

	if (nOpenFlags & (CFile::modeWrite | CFile::modeReadWrite))
		pFile->m_dwLength = 0; // can't estimate this
	else
		pFile->m_dwLength = pFile->GetLength();

	return pFile;
}

int AMswDoc::MapType(int nType)
{
   return (nType == DocType::RD_OEMTEXT) ? DocType::RD_TEXT : nType;
}

void AMswDoc::OnFileSendMail()
{
	if (m_strTitle.Find(_T('.')) == -1)
	{  // add the extension because the default extension will be wrong
		CString strOldTitle = m_strTitle;
		m_strTitle += GetExtFromType(m_nDocType);
		CRichEditDoc::OnFileSendMail();
		m_strTitle = strOldTitle;
	}
	else
		CRichEditDoc::OnFileSendMail();
}

void AMswDoc::OnFileSaveCopyAs()
{
   if (!DoSave(NULL, FALSE))
      TRACE(traceAppMsg, 0, "Warning: File save-copy-as failed.\n");
}

int AMswDoc::GetStreamFormat() const
{  // valid stream formats are 
   switch (m_nDocType)
   {
   case DocType::RD_RICHTEXT:    return SF_RTF;
   case DocType::RD_URICHTEXT:   return SF_RTF | SF_TEXT | SF_UNICODE;
   case DocType::RD_UTEXT:       return SF_TEXT | SF_UNICODE;
   default:                      return SF_TEXT;
   }
}

CRichEditCntrItem* AMswDoc::CreateClientItem(REOBJECT* preo) const
{  // cast away constness of this
	return new CMswCntrItem(preo, (AMswDoc*)this);
}
