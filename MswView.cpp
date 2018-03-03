// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"

// MLMehr - following fix suggested for errors in Afximpl.h involving HRAWINPUT definition missing
#define HRAWINPUT DWORD

#include <../Src/Mfc/AfxImpl.h>

#include "Bookmark.h"
#include "colorlis.h"
#include "formatba.h"
#include "formatpa.h"
#include "GotoLineDlg.h"
#include "mainfrm.h"
#include "mswdoc.h"
#include "mswview.h"
#include "rtfField.h"
#include "rtfMemFile.h"
#include "strings.h"
#include "ScriptQueueDlg.h"
#include "StyleListDialog.h"
#include "FindReplaceDlg.h"
#include "FontSizeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CLIPFORMAT cfRTF;
static const int kGotoBookmarkItemPos  = 1;

static const int kAutoScrollFrequency  = 200;  // milliseconds
static const BYTE kAutoScrollHeight = 10;  // number of pixels from top and bottom that mark the 'auto scroll zone'


bool CCharFormat::operator == (const CHARFORMAT2& cf) const
{
   const int releventEffects = CFE_BOLD | CFE_ITALIC | CFE_UNDERLINE | CFE_STRIKEOUT;
   return
      (bCharSet == cf.bCharSet) && 
      (bPitchAndFamily == cf.bPitchAndFamily) && 
      (crTextColor == cf.crTextColor) && 
      ((dwEffects & releventEffects) == (cf.dwEffects & releventEffects)) && 
      (yHeight == cf.yHeight) && 
      (yOffset == cf.yOffset) && 
      (0 == ::_tcscmp(cf.szFaceName, szFaceName));
}

void CCharFormat::SetFaceName(LPCTSTR name)
{
   ASSERT(NULL != name);
   ::_tcsncpy_s(szFaceName, mCountOf(szFaceName), name, ::_tcslen(name));
}

bool CParaFormat::operator == (PARAFORMAT2& pf)
{
	if (dwMask != pf.dwMask || wNumbering != pf.wNumbering || wEffects != pf.wEffects || 
      dxStartIndent != pf.dxStartIndent || dxRightIndent != pf.dxRightIndent 
      || dxOffset != pf.dxOffset || cTabCount != pf.cTabCount)
	{
		return FALSE;
	}
	for (int i=0;i<pf.cTabCount;i++)
	{
		if (rgxTabs[i] != pf.rgxTabs[i])
			return FALSE;
	}
	return TRUE;
}

IMPLEMENT_DYNCREATE(AMswView, CRichEditView)

//WM_SETTINGCHANGE -- default printer might have changed
//WM_FONTCHANGE -- pool of fonts changed
//WM_DEVMODECHANGE -- printer settings changes

BEGIN_MESSAGE_MAP(AMswView, CRichEditView)
	//{{AFX_MSG_MAP(AMswView)
	ON_WM_CREATE()
	ON_COMMAND(rCmdFilePageSetup, OnPageSetup)
	ON_COMMAND(rCmdFormatPara, OnFormatParagraph)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_MEASUREITEM()
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_WM_PALETTECHANGED()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_SETTINGCHANGE()
	ON_WM_SIZE()
   ON_WM_DROPFILES()
	ON_COMMAND(rCmdCheckSpelling, OnCheckSpelling)
	ON_COMMAND(rCmdSpellerOptions, OnSpellerOptions)
	ON_UPDATE_COMMAND_UI(rCmdSpellerOptions, OnUpdateSpellerOptions)
   ON_COMMAND(rCmdAutoCorrectOptions, OnAutoCorrectOptions)
   ON_UPDATE_COMMAND_UI(rCmdAutoCorrectOptions, OnUpdateAutoCorrectOptions)
	ON_COMMAND(ID_CHAR_BOLD, OnCharBold)
	ON_UPDATE_COMMAND_UI(ID_CHAR_BOLD, OnUpdateCharBold)
	ON_COMMAND(ID_CHAR_ITALIC, OnCharItalic)
	ON_UPDATE_COMMAND_UI(ID_CHAR_ITALIC, OnUpdateCharItalic)
	ON_COMMAND(ID_CHAR_UNDERLINE, OnCharUnderline)
	ON_UPDATE_COMMAND_UI(ID_CHAR_UNDERLINE, OnUpdateCharUnderline)
	ON_COMMAND(ID_PARA_CENTER, OnParaCenter)
	ON_UPDATE_COMMAND_UI(ID_PARA_CENTER, OnUpdateParaCenter)
	ON_COMMAND(ID_PARA_LEFT, OnParaLeft)
	ON_UPDATE_COMMAND_UI(ID_PARA_LEFT, OnUpdateParaLeft)
	ON_COMMAND(ID_PARA_RIGHT, OnParaRight)
	ON_UPDATE_COMMAND_UI(ID_PARA_RIGHT, OnUpdateParaRight)
	ON_COMMAND(rCmdColor16, OnColorDefault)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
	ON_UPDATE_COMMAND_UI(rCmdCheckSpelling, OnUpdateCheckSpelling)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(rCmdFormatBullets, CRichEditView::OnBullet)
	ON_UPDATE_COMMAND_UI(rCmdFormatBullets, CRichEditView::OnUpdateBullet)
	ON_COMMAND(rCmdFilePrintPreview, CRichEditView::OnFilePrintPreview)
	ON_COMMAND_RANGE(rCmdColor0, rCmdColor16, OnColorPick)
	ON_COMMAND_RANGE(rCmdCuePointer0, rCmdCuePointer9, OnCuePointer)
	ON_COMMAND_RANGE(0x0FFF, 0x103F, OnSpellCorrection)
	ON_EN_CHANGE(AFX_IDW_PANE_FIRST, OnEditChange)
	ON_WM_MOUSEACTIVATE()
	ON_REGISTERED_MESSAGE(AMswApp::sPrinterChangedMsg, OnPrinterChangedMsg)
	ON_NOTIFY(FN_GETFORMAT, rCmdViewFormatbar, OnGetCharFormat)
	ON_NOTIFY(FN_SETFORMAT, rCmdViewFormatbar, OnSetCharFormat)
	ON_NOTIFY(NM_SETFOCUS, rCmdViewFormatbar, OnBarSetFocus)
	ON_NOTIFY(NM_KILLFOCUS, rCmdViewFormatbar, OnBarKillFocus)
	ON_NOTIFY(NM_RETURN, rCmdViewFormatbar, OnBarReturn)
   ON_COMMAND(rCmdSizeLarger, &OnFontScale)
   ON_COMMAND(rCmdSizeSmaller, &OnFontScale)
   ON_COMMAND(ID_FORMAT_STYLES, &OnFormatStyle)

   ON_COMMAND_RANGE(rCmdGotoBkMk1, rCmdGotoBkMk1 + ABookmark::kMaxCount, &OnGotoBookmark)
   ON_COMMAND_RANGE(rCmdStyle1, rCmdStyle1 + AStyle::kMaxCount, &OnStyle)
   ON_COMMAND(rCmdSetBkMk1, &OnSetBookmark)
   ON_UPDATE_COMMAND_UI(rCmdSetBkMk1, &OnUpdateSetBookmark)
   ON_COMMAND(rCmdClearBkMks, &OnClearBookmarks)
   ON_UPDATE_COMMAND_UI(rCmdClearBkMks, &OnUpdateClearBookmarks)
   ON_COMMAND(rCmdNextBkmk, &OnNextBookmark)
   ON_UPDATE_COMMAND_UI(rCmdNextBkmk, &OnUpdateNextBookmark)
   ON_COMMAND(rCmdPrevBkmk, &OnPrevBookmark)
   ON_UPDATE_COMMAND_UI(rCmdPrevBkmk, &OnUpdatePrevBookmark)

   ON_COMMAND(rCmdTogglePaperclip, &OnSetPaperclip)
   ON_UPDATE_COMMAND_UI(rCmdTogglePaperclip, &OnUpdateSetPaperclip)
   ON_COMMAND(rCmdClearPclips, &OnClearPaperclips)
   ON_UPDATE_COMMAND_UI(rCmdClearPclips, &OnUpdateClearPaperclips)
   ON_COMMAND(rCmdNextPclip, &OnNextPaperclip)
   ON_UPDATE_COMMAND_UI(rCmdNextPclip, &OnUpdateNextPaperclip)
   ON_COMMAND(rCmdPrevPclip, &OnPrevPaperclip)
   ON_UPDATE_COMMAND_UI(rCmdPrevPclip, &OnUpdatePrevPaperclip)

   ON_COMMAND(rCmdGotoLine, &OnGotoLine)
   ON_CONTROL_REFLECT(EN_CHANGE, &OnEnChange)
   ON_NOTIFY_REFLECT(EN_SELCHANGE, &OnEnSelchange)
   ON_WM_PAINT()
   ON_WM_LBUTTONDOWN()
   ON_WM_LBUTTONUP()
   ON_WM_MOUSEMOVE()
   ON_WM_SYSCOLORCHANGE()
   ON_WM_KEYUP()
   ON_WM_CHAR()
   ON_CONTROL_REFLECT(EN_VSCROLL, &OnEnVscroll)
   ON_WM_VSCROLL()
   ON_COMMAND(rCmdBookmarks, &OnBookmarks)
	ON_COMMAND(rCmdEditFind, &OnFindReplace)
	ON_COMMAND(rCmdFormatFontBtn, &OnFormatFont)
   ON_COMMAND(ID_STYLE_1SPACE, &OnSpacing)
   ON_COMMAND(ID_STYLE_15SPACE, &OnSpacing)
   ON_COMMAND(ID_STYLE_2SPACE, &OnSpacing)
   ON_COMMAND(rCmdStylePlain, &OnStylePlain)

   ON_COMMAND(rCmdSize12, &OnSize)
   ON_COMMAND(rCmdSize18, &OnSize)
   ON_COMMAND(rCmdSize24, &OnSize)
   ON_COMMAND(rCmdSize36, &OnSize)
   ON_COMMAND(rCmdSize40, &OnSize)
   ON_COMMAND(rCmdSize48, &OnSize)
   ON_COMMAND(rCmdSize64, &OnSize)
   ON_COMMAND(rCmdSize72, &OnSize)
   ON_COMMAND(rCmdSize96, &OnSize)
   ON_UPDATE_COMMAND_UI(rCmdSize12, &OnUpdateSize)
   ON_UPDATE_COMMAND_UI(rCmdSize18, &OnUpdateSize)
   ON_UPDATE_COMMAND_UI(rCmdSize24, &OnUpdateSize)
   ON_UPDATE_COMMAND_UI(rCmdSize36, &OnUpdateSize)
   ON_UPDATE_COMMAND_UI(rCmdSize40, &OnUpdateSize)
   ON_UPDATE_COMMAND_UI(rCmdSize48, &OnUpdateSize)
   ON_UPDATE_COMMAND_UI(rCmdSize64, &OnUpdateSize)
   ON_UPDATE_COMMAND_UI(rCmdSize72, &OnUpdateSize)
   ON_UPDATE_COMMAND_UI(rCmdSize96, &OnUpdateSize)
   ON_COMMAND(rCmdSizeOther, &OnSizeOther)
   ON_COMMAND(rCmdEditSelectAll, &OnSelectAll)
   ON_REGISTERED_MESSAGE(AMswApp::sShowRulerMsg, OnShowRuler)
   ON_REGISTERED_MESSAGE(AMswApp::sSetScrollMarginsMsg, OnSetScrollMargins)
   ON_REGISTERED_MESSAGE(AMswApp::sRedrawCueBarMsg, OnRedrawCueBar)
   ON_COMMAND(rCmdTimerToArt, &OnTimerToArt)
   ON_COMMAND(rCmdScrollLineUp, &OnScrollLineUp)
   ON_COMMAND(rCmdScrollLineDown, &OnScrollLineDown)
   ON_REGISTERED_MESSAGE(AMswApp::sInverseEditModeMsg, &OnInverseEditMode)

   ON_COMMAND(rCmdEditPaste, &OnEditPaste)
   ON_COMMAND(rCmdEditPasteSpecial, &OnEditPasteSpecial)
   ON_UPDATE_COMMAND_UI(rCmdEditPaste, &OnUpdatePaste)
   ON_UPDATE_COMMAND_UI(rCmdEditPasteSpecial, &OnUpdatePaste)
   ON_UPDATE_COMMAND_UI_RANGE(rCmdColor0, rCmdColor19, OnUpdateColor)
END_MESSAGE_MAP()

AMswView::AMswView() :
	m_uTimerID(0),
   fAutoScrollTimer(0),
	m_bDelayUpdateItems(FALSE),
	m_bOnBar(FALSE),
	m_bInPrint(FALSE),
   fTextLength(-1),
   fRtfHelper(false),
   fArt(0),
   fDragSelecting(false),
   fRulerHeight(0)
{
	m_bSyncCharFormat = m_bSyncParaFormat = TRUE;
	m_nPasteType = 0;
	m_rectMargin = theApp.m_rectPageMargin;

   fRtfHelper.fParent = this;
   fRtfHelper.fEdit = &this->GetRichEditCtrl();
   m_nWordWrap = WrapToWindow;
}

AMswView::~AMswView()
{
   TRACE("~AMswView::m_dwRef = %d\n", this->m_dwRef);
   if (theApp.fScriptQueue.Remove(this))
      theApp.fScriptQueueDlg.RefreshList();
}

BOOL AMswView::PreCreateWindow(CREATESTRUCT& cs)
{
	BOOL bRes = CRichEditView::PreCreateWindow(cs);
   cs.style |= ES_SELECTIONBAR;
	return bRes;
}

BOOL AMswView::IsFormatText()
{
	// this function checks to see if any formatting is not default text
	BOOL bRes = FALSE;
	CHARRANGE cr;
	CCharFormat cf;
	GetRichEditCtrl().GetSel(cr);
	GetRichEditCtrl().HideSelection(TRUE, FALSE);
	GetRichEditCtrl().SetSel(0,-1);

	if (!(GetRichEditCtrl().GetSelectionType() & (SEL_OBJECT|SEL_MULTIOBJECT)))
	{
		GetRichEditCtrl().GetSelectionCharFormat(cf);
		if (cf == theApp.fDefaultFont)
		{
      	CParaFormat pf;
			GetRichEditCtrl().GetParaFormat(pf);
			if (pf == m_defParaFormat) //compared using CParaFormat::operator==
				bRes = TRUE;
		}
	}

	GetRichEditCtrl().SetSel(cr);
	GetRichEditCtrl().HideSelection(FALSE, FALSE);
	return bRes;
}

HMENU AMswView::GetContextMenu(WORD, LPOLEOBJECT, CHARRANGE*)
{
   CRichEditCntrItem* pItem = GetSelectedItem();
	if (pItem == NULL || !pItem->IsInPlaceActive())
	{
		CMenu menuText;
		VERIFY(menuText.LoadMenu(rMenuTextPopup));
		CMenu* pMenuPopup = menuText.GetSubMenu(0);
      ASSERT(NULL != pMenuPopup);
      ASSERT_VALID(pMenuPopup);
      VERIFY(menuText.RemoveMenu(0, MF_BYPOSITION));
		return pMenuPopup->Detach();
	}
	return NULL;
}

void AMswView::SetUpdateTimer()
{
	if (m_uTimerID != 0) // if outstanding timer kill it
		KillTimer(m_uTimerID);
	m_uTimerID = SetTimer(1, 1000, NULL); //set a timer for 1000 milliseconds
	if (m_uTimerID == 0) // no timer available so force update now
		GetDocument()->UpdateAllItems(NULL);
	else
		m_bDelayUpdateItems = TRUE;
}

void AMswView::DeleteContents()
{
	ASSERT_VALID(this);
	ASSERT(m_hWnd != NULL);
	CRichEditView::DeleteContents();
	this->SetDefaultFont();
}

void AMswView::SetDefaultFont()
{
	ASSERT_VALID(this);
	ASSERT(m_hWnd != NULL);
	m_bSyncCharFormat = m_bSyncParaFormat = TRUE;
   fRtfHelper.SetDefaultFont();
}

void AMswView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
	CRichEditView::OnPrint(pDC, pInfo);
	if (pInfo != NULL && pInfo->m_bPreview)
		DrawMargins(pDC);
}

void AMswView::DrawMargins(CDC* pDC)
{
	if (pDC->m_hAttribDC != NULL)
	{
		CRect rect;
		rect.left = m_rectMargin.left;
		rect.right = m_sizePaper.cx - m_rectMargin.right;
		rect.top = m_rectMargin.top;
		rect.bottom = m_sizePaper.cy - m_rectMargin.bottom;
		//rect in twips
		int logx = ::GetDeviceCaps(pDC->m_hDC, LOGPIXELSX);
		int logy = ::GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
		rect.left = MulDiv(rect.left, logx, 1440);
		rect.right = MulDiv(rect.right, logx, 1440);
		rect.top = MulDiv(rect.top, logy, 1440);
		rect.bottom = MulDiv(rect.bottom, logy, 1440);
		CPen pen(PS_DOT, 0, pDC->GetTextColor());
		CPen* ppen = pDC->SelectObject(&pen);
		pDC->MoveTo(0, rect.top);
		pDC->LineTo(10000, rect.top);
		pDC->MoveTo(rect.left, 0);
		pDC->LineTo(rect.left, 10000);
		pDC->MoveTo(0, rect.bottom);
		pDC->LineTo(10000, rect.bottom);
		pDC->MoveTo(rect.right, 0);
		pDC->LineTo(rect.right, 10000);
		pDC->SelectObject(ppen);
	}
}

BOOL AMswView::OnPreparePrinting(CPrintInfo* pInfo)
{
	return DoPreparePrinting(pInfo);
}

inline int roundleast(int n)
{
	int mod = n%10;
	n -= mod;
	if (mod >= 5)
		n += 10;
	else if (mod <= -5)
		n -= 10;
	return n;
}

static void RoundRect(LPRECT r1)
{
	r1->left = roundleast(r1->left);
	r1->right = roundleast(r1->right);
	r1->top = roundleast(r1->top);
	r1->bottom = roundleast(r1->bottom);
}

static void MulDivRect(LPRECT r1, LPRECT r2, int num, int div)
{
	r1->left = MulDiv(r2->left, num, div);
	r1->top = MulDiv(r2->top, num, div);
	r1->right = MulDiv(r2->right, num, div);
	r1->bottom = MulDiv(r2->bottom, num, div);
}

void AMswView::OnPageSetup()
{
	CPageSetupDialog dlg;
	PAGESETUPDLG& psd = dlg.m_psd;
	BOOL bMetric = theApp.GetUnits() == 1; //centimeters
	psd.Flags |= PSD_MARGINS | (bMetric ? PSD_INHUNDREDTHSOFMILLIMETERS :
		PSD_INTHOUSANDTHSOFINCHES);
	int nUnitsPerInch = bMetric ? 2540 : 1000;
	MulDivRect(&psd.rtMargin, m_rectMargin, nUnitsPerInch, 1440);
	RoundRect(&psd.rtMargin);

   // get the current device from the app
	PRINTDLG pd;
   ::memset(&pd, 0, sizeof(pd));
	theApp.GetPrinterDeviceDefaults(&pd);
	psd.hDevNames = pd.hDevNames;
	psd.hDevMode = pd.hDevMode;
	if (dlg.DoModal() == IDOK)
	{
		RoundRect(&psd.rtMargin);
		MulDivRect(m_rectMargin, &psd.rtMargin, 1440, nUnitsPerInch);
		theApp.m_rectPageMargin = m_rectMargin;
		theApp.SelectPrinter(psd.hDevNames, psd.hDevMode);
		theApp.NotifyPrinterChanged();
	}
}

int AMswView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CRichEditView::OnCreate(lpCreateStruct) == -1)
		return -1;

   theApp.m_listPrinterNotify.AddTail(m_hWnd);

	if (!this->CreateRulerBar())
		return -1;

   if (theApp.fWordSel)
		GetRichEditCtrl().SetOptions(ECOOP_OR, ECO_AUTOWORDSELECTION);
	else
		GetRichEditCtrl().SetOptions(ECOOP_AND, ~(DWORD)ECO_AUTOWORDSELECTION);

   // this will disable drag/dropping of OLE objects, but also of 
   // text within the document
   //::RevokeDragDrop(GetRichEditCtrl().m_hWnd);

	GetRichEditCtrl().GetParaFormat(m_defParaFormat);
   GetRichEditCtrl().SetEventMask(ENM_SELCHANGE | ENM_CHANGE | ENM_SCROLL);
	m_defParaFormat.cTabCount = 0;

   this->ModifyStyle(0, WS_CLIPCHILDREN);
	return 0;
}

void AMswView::OnFormatParagraph()
{
	AFormatParaDlg dlg(this->GetParaFormatSelection());
	if (dlg.DoModal() == IDOK)
		VERIFY(this->SetParaFormat(dlg.m_pf));
}

void AMswView::OnTextNotFound(LPCTSTR lpStr)
{
	ASSERT_VALID(this);
	AfxMessageBox(IDS_FINISHED_SEARCH,MB_OK|MB_ICONINFORMATION);
	CRichEditView::OnTextNotFound(lpStr);
}

void AMswView::OnColorPick(UINT nID)
{
	CRichEditView::OnColorPick(AColorMenu::GetColor(nID));
}

void AMswView::OnCuePointer(UINT nID)
{
   theApp.fCuePointer = BYTE(nID - rCmdCuePointer0);
   this->OnRedrawCueBar(0,0);
}

void AMswView::OnSpellCorrection(UINT)
{  // the spell checker component handles this. We have to define it in order
   // for the menu items to be enabled, however.
}

void AMswView::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == m_uTimerID)
	{  // one-shot timer
      this->KillTimer(m_uTimerID);
		m_uTimerID = 0;
		if (m_bDelayUpdateItems)
			GetDocument()->UpdateAllItems(NULL);
		m_bDelayUpdateItems = FALSE;
	}
   else if (nIDEvent == fAutoScrollTimer)
   {  // auto-scroll timer
      CRect r;
      this->GetClientRect(r);
      CPoint mouse;
      VERIFY(::GetCursorPos(&mouse));
      this->ScreenToClient(&mouse);
      const bool down = (mouse.y > (r.bottom - kAutoScrollHeight));
      const bool up = (mouse.y < (fRulerHeight + kAutoScrollHeight));
      if (up || down)
      {
         CRichEditCtrl& edit = GetRichEditCtrl();
         // set new selection up or down one line
         long start = 0, end = 0;
         edit.GetSel(start, end);
         ASSERT(start <= end);
         if (fSelStart < 0)   // just set this once
            fSelStart = up ? end : start;

         if (start != fSelStart)
            std::swap(start, end);

         long line = edit.LineFromChar(end);
         if ((line <= 0) || (line >= edit.GetLineCount()-1))
            return;  // nothing left to scroll

         if (up)
            end = edit.LineIndex(line - 1);
         else
            end = edit.LineIndex(line + 2) - 1;
         edit.SetSel(start, end);
         TRACE("auto-scroll (%s) %d:%d\n", up ? _T("up") : _T("down"), start, end);

         edit.SendMessage(EM_LINESCROLL, 0, up ? -1 : 1);
      }
   }
   else
   {  // not our timer
		CRichEditView::OnTimer(nIDEvent);
   }
}

void AMswView::OnEditChange()
{
	SetUpdateTimer();
}

void AMswView::OnDestroy()
{
	POSITION pos = theApp.m_listPrinterNotify.Find(m_hWnd);
	ASSERT(pos != NULL);
	theApp.m_listPrinterNotify.RemoveAt(pos);

	CRichEditView::OnDestroy();

	if (m_uTimerID != 0) // if outstanding timer kill it
		OnTimer(m_uTimerID);
	ASSERT(m_uTimerID == 0);
}

void AMswView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	CRichEditView::CalcWindowRect(lpClientRect, nAdjustType);

	if ((nAdjustType != 0) && (GetStyle() & WS_VSCROLL))
		lpClientRect->right--;
}

void AMswView::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
	lpMIS->itemID = (UINT)(WORD)lpMIS->itemID;
	CRichEditView::OnMeasureItem(nIDCtl, lpMIS);
}

void AMswView::OnFilePrint()
{
	// don't allow winini changes to occur while printing
	m_bInPrint = TRUE;
	CRichEditView::OnFilePrint();
	// printer may have changed
	theApp.NotifyPrinterChanged(); // this will cause a GetDocument()->PrinterChanged();
	m_bInPrint = FALSE;
}

int AMswView::OnMouseActivate(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bOnBar)
	{
		SetFocus();
		return MA_ACTIVATEANDEAT;
	}
	else
		return CRichEditView::OnMouseActivate(pWnd, nHitTest, message);
}

LONG AMswView::OnPrinterChangedMsg(UINT, LONG)
{
	CDC dc;
	AfxGetApp()->CreatePrinterDC(dc);
	OnPrinterChanged(dc);
	return 0;
}

static void ForwardPaletteChanged(HWND hWndParent, HWND hWndFocus)
{
	// this is a quick and dirty hack to send the WM_QUERYNEWPALETTE to a window that is interested
	HWND hWnd = NULL;
	for (hWnd = ::GetWindow(hWndParent, GW_CHILD); hWnd != NULL; hWnd = ::GetWindow(hWnd, GW_HWNDNEXT))
	{
		if (hWnd != hWndFocus)
		{
			::SendMessage(hWnd, WM_PALETTECHANGED, (WPARAM)hWndFocus, 0L);
			ForwardPaletteChanged(hWnd, hWndFocus);
		}
	}
}

void AMswView::OnPaletteChanged(CWnd* pFocusWnd)
{
	ForwardPaletteChanged(m_hWnd, pFocusWnd->GetSafeHwnd());
	// allow the richedit control to realize its palette
	// remove this if if richedit fixes their code so that
	// they don't realize their palette into foreground
	if (::GetWindow(m_hWnd, GW_CHILD) == NULL)
		CRichEditView::OnPaletteChanged(pFocusWnd);
}

static BOOL FindQueryPalette(HWND hWndParent)
{
	// this is a quick and dirty hack to send the WM_QUERYNEWPALETTE to a window that is interested
	HWND hWnd = NULL;
	for (hWnd = ::GetWindow(hWndParent, GW_CHILD); hWnd != NULL; hWnd = ::GetWindow(hWnd, GW_HWNDNEXT))
	{
		if (::SendMessage(hWnd, WM_QUERYNEWPALETTE, 0, 0L))
			return TRUE;
		else if (FindQueryPalette(hWnd))
			return TRUE;
	}
	return FALSE;
}

BOOL AMswView::OnQueryNewPalette()
{
	if(FindQueryPalette(m_hWnd))
		return TRUE;
	return CRichEditView::OnQueryNewPalette();
}

void AMswView::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CRichEditView::OnSettingChange(uFlags, lpszSection);
	//printer might have changed
	if (!m_bInPrint)
	{
		if (lstrcmpi(lpszSection, _T("windows")) == 0)
			theApp.NotifyPrinterChanged(TRUE); // force update to defaults
	}
}

void AMswView::OnSize(UINT nType, int cx, int cy)
{
   CRichEditView::OnSize(nType, cx, cy);

   int x = 0;
   fRulerHeight = 0;
   if (::IsWindow(m_wndRulerBar.m_hWnd) && m_wndRulerBar.IsVisible())
   {
      fRulerHeight = m_wndRulerBar.CalcFixedLayout(FALSE, TRUE).cy;
      cx += ::GetSystemMetrics(SM_CXVSCROLL);
      m_wndRulerBar.MoveWindow(0, 0, cx, fRulerHeight - 1);
   }

   fRtfHelper.Size(x, fRulerHeight, cx, cy);
}

void AMswView::OnGetCharFormat(NMHDR* pNMHDR, LRESULT* pRes)
{
	ASSERT(pNMHDR != NULL);
	ASSERT(pRes != NULL);
	((CHARHDR*)pNMHDR)->cf = GetCharFormatSelection();
	*pRes = 1;
}

void AMswView::OnSetCharFormat(NMHDR* pNMHDR, LRESULT* pRes)
{
	ASSERT(pNMHDR != NULL);
	ASSERT(pRes != NULL);
	SetCharFormat(((CHARHDR*)pNMHDR)->cf);
	*pRes = 1;
}

void AMswView::OnBarSetFocus(NMHDR*, LRESULT*)
{
	m_bOnBar = TRUE;
}

void AMswView::OnBarKillFocus(NMHDR*, LRESULT*)
{
	m_bOnBar = FALSE;
}

void AMswView::OnBarReturn(NMHDR*, LRESULT* )
{
	SetFocus();
}

void AMswView::OnUpdateSpellerOptions(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(NULL != gSpeller.GetSafeHwnd());
}

void AMswView::OnUpdateAutoCorrectOptions(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(NULL != gSpeller.GetSafeHwnd());
}

void AMswView::OnSpellerOptions() 
{
   if (gSpeller.OptionsDlg())
   {  // save the new options
      CWinApp* app = ::AfxGetApp();
      app->WriteProfileInt(gStrSpeller, gStrAsYouType, gSpeller.get_CheckSpellingAsYouType());
      app->WriteProfileInt(gStrSpeller, gStrAlwaysSuggest, gSpeller.get_AlwaysSuggest());
      app->WriteProfileInt(gStrSpeller, gStrFromMain, gSpeller.get_SuggestFromMainDictionariesOnly());
      app->WriteProfileInt(gStrSpeller, gStrIgnoreUpper, gSpeller.get_IgnoreWordsInUppercase());
      app->WriteProfileInt(gStrSpeller, gStrIgnoreNumbers, gSpeller.get_IgnoreWordsWithNumbers());
      app->WriteProfileInt(gStrSpeller, gStrIgnoreLinks, gSpeller.get_IgnoreInternetAndFileAddresses());

      // save loaded dictionaries
      CString key;
      int nDicts = gSpeller.get_DictionaryCount();
      for (long i = 0; ; ++i)
      {
         key.Format(_T("%s%d"), (LPCTSTR)gStrDict, i);
         if (i < nDicts)
            app->WriteProfileString(gStrSpeller, key, gSpeller.GetDictionaryPath(i + 1));
         else if (!app->GetProfileString(gStrSpeller, key).IsEmpty())
            app->WriteProfileString(gStrSpeller, key, NULL);
         else
            break;
      }
      nDicts = gSpeller.get_CustomDictionaryCount();
      for (long i = 0; ; ++i)
      {
         key.Format(_T("%s%d"), (LPCTSTR)gStrCustomDict, i);
         if (i < nDicts)
            app->WriteProfileString(gStrSpeller, key, gSpeller.GetCustomDictionaryPath(i + 1));
         else if (!app->GetProfileString(gStrSpeller, key).IsEmpty())
            app->WriteProfileString(gStrSpeller, key, NULL);
         else
            break;
      }
   }
}

void AMswView::OnAutoCorrectOptions()
{
   if (gSpeller.AutoCorrectOptionsDlg())
   {  // save the new options
      CWinApp* app = ::AfxGetApp();
      app->WriteProfileInt(gStrSpeller, gStrCorrect2Caps, gSpeller.get_CorrectTwoInitCaps());
      app->WriteProfileInt(gStrSpeller, gStrCapFirst, gSpeller.get_CapitalizeSentence());
      app->WriteProfileInt(gStrSpeller, gStrCapsLock, gSpeller.get_CorrectCapsLock());
      app->WriteProfileInt(gStrSpeller, gStrReplaceText, gSpeller.get_AutoCorrectTextAsYouType());
      app->WriteProfileInt(gStrSpeller, gStrAddWords, gSpeller.get_AutoAddWordsToExceptionList());

      gSpeller.SaveInitializationFile(theApp.GetExeDir() + gStrAutoCorrect);
   }
}

void AMswView::OnUpdateCheckSpelling(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((NULL != gSpeller.GetSafeHwnd()) &&
    (Utils::GetTextLenth(this->GetRichEditCtrl().m_hWnd) > 0));
}

void AMswView::OnCheckSpelling() 
{
   long begin = 0, end = 0;
   this->GetSel(begin, end);
   const bool selection = (end != begin);
   gSpeller.CheckTextControl(selection ? CSpellchecker1::scrSelectionOnly : 
      CSpellchecker1::scrEntireDocument);
}

BOOL AMswView::CreateRulerBar()
{
   DWORD style = WS_CHILD | CBRS_TOP | CBRS_TOOLTIPS;
   if (theApp.fShowRuler)
      style |= WS_VISIBLE;

   if (!m_wndRulerBar.Create(this, style, rCmdViewRuler))
	{
		TRACE("Failed to create ruler\n");
		return FALSE;      // fail to create
	}

	return TRUE;
}

LONG AMswView::OnShowRuler(UINT, LONG)
{
   if (::IsWindow(m_wndRulerBar.m_hWnd))
   {
      m_wndRulerBar.ShowWindow(theApp.fShowRuler ? SW_SHOW : SW_HIDE);

      CRect client;
      this->GetClientRect(client);
      this->OnSize(SIZE_RESTORED, client.Width(), client.Height());
   }
   return 0;
}

void AMswView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
   CRichEditView::OnActivateView(bActivate, pActivateView, pDeactiveView);
   if (bActivate)
   {  // set speller target
      if (NULL != gSpeller.GetSafeHwnd())
         gSpeller.put_LinkedTextControlHandle((long)this->GetRichEditCtrl().m_hWnd);

      // update the script queue dialog
      if (theApp.fScriptQueueDlg.IsWindowVisible())
         theApp.fScriptQueueDlg.Invalidate();
   }
}

void AMswView::OnFontScale()
{
   this->SetRedraw(FALSE);
   const bool bigger = (rCmdSizeLarger == LOWORD(this->GetCurrentMessage()->wParam));

   CRichEditCtrl& edit = GetRichEditCtrl();
	CHARRANGE selection;
   edit.GetSel(selection);
   if (selection.cpMin == selection.cpMax)
   {  // no selection, so select all
   	edit.HideSelection(TRUE, FALSE);
      edit.SetSel(0, -1);
   }

   GetRichEditCtrl().SendMessage(EM_SETFONTSIZE, bigger ? 2 : -2);

   // restore the original selection
   edit.SetSel(selection);
	edit.HideSelection(FALSE, FALSE);
   this->SetRedraw(TRUE);
   this->Invalidate();
}

void AMswView::OnFormatStyle()
{
	CStyleListDialog().DoModal();
}


// {begin
// ripped straight from MFC so we can call our own 'GetStreamFormat' to recognize
// UNICODE files
void AMswView::Serialize(CArchive& ar)
	// Read and write CRichEditView object to archive, with length prefix.
{
	ASSERT_VALID(this);
	ASSERT(m_hWnd != NULL);
	Stream(ar, FALSE);
	ASSERT_VALID(this);
}

class _afxRichEditCookie
{
public:
	CArchive& m_ar;
	DWORD m_dwError;
	_afxRichEditCookie(CArchive& ar) : m_ar(ar) {m_dwError=0;}
};

void AMswView::ReadMarks(CRtfMemFile& file)
{  // first clear out the old marks
   fRtfHelper.fBookmarks.clear();
   fRtfHelper.fPaperclips.clear();
   
   // read the file into memory so we can find the bookmarks and paperclips
   ARtfFields fields;
   const size_t nHeaderSize = file.GetHeaderSize();
   file.BuildRtfFieldArray(fields, nHeaderSize);
   for (size_t i = 0; i < fields.size(); ++i)
   {
      CString name = fields[i].fParameters;
      if (0 == fields[i].fName.CompareNoCase(gStrBookmark))
         VERIFY(fRtfHelper.fBookmarks.Add(ABookmark(name, fields[i].fTextPos)));
      else if (0 == fields[i].fName.CompareNoCase(gStrPaperclip))
         VERIFY(fRtfHelper.fPaperclips.Toggle(AMarker(fields[i].fTextPos)));
   }
}

void AMswView::WriteMarks(CRtfMemFile& file)
{  // first write the control contents to a memory file
   // write them in reverse order so that when two marks
   // occur at the same location, the one that occurs
   // first in the list will be written first in the file.
   CStringA field;
   const size_t nHeaderSize = file.GetHeaderSize();
   for (ABookmarks::reverse_iterator i = fRtfHelper.fBookmarks.rbegin();
      i != fRtfHelper.fBookmarks.rend(); ++i)
   {
      size_t pos = file.GetFilePosFromTextPos(i->fPos, nHeaderSize);
      field.Format("%s%s %s}", gStrRtfField, gStrBookmark, (LPCSTR)CStringA((LPCTSTR)i->fName));
      file.Insert(pos, field);
   }

   field.Format("%s%s}", gStrRtfField, gStrPaperclip);
   for (size_t i = 0; i < fRtfHelper.fPaperclips.size(); ++i)
   {
      size_t pos = file.GetFilePosFromTextPos(
         fRtfHelper.fPaperclips[i].fPos, nHeaderSize);
      file.Insert(pos, field);
   }
}

void AMswView::Stream(CArchive& ar, BOOL bSelection)
{
	int nFormat = this->GetDocument()->GetStreamFormat();
   const bool unicodeText = (nFormat & (SF_TEXT | SF_UNICODE)) == (SF_TEXT | SF_UNICODE);

   if (bSelection)
		nFormat |= SFF_SELECTION;

   CRtfMemFile file;
   if (ar.IsStoring())
   {  // writing
      file.StreamToFile(this->GetRichEditCtrl(), nFormat);

      if ((SF_RTF & nFormat) && (theApp.fInverseEditMode))
         file.SetTextColor(kBlack, false);

      if (SF_RTF & nFormat)
         this->WriteMarks(file);

      if (unicodeText)
      {  // write the byte order marker
         const WORD bom = 0xFEFF;
         ar.Write(&bom, sizeof(bom));
      }

      ar.Write(file.GetBuffer(), (UINT)file.GetLength());
   }
	else
	{  // reading
      if (unicodeText)
      {  // read the byte order marker
         WORD bom;
         VERIFY(sizeof(bom) == ar.Read(&bom, sizeof(bom)));
         ASSERT(0xFEFF == bom);
      }

      BYTE buffer[4096];
      UINT nBufferRead = 0;
      while ((nBufferRead = ar.Read(buffer, sizeof(buffer))) > 0)
         file.Write(buffer, nBufferRead);

      if (SF_RTF & nFormat)
         this->ReadMarks(file);

      file.StreamFromFile(this->GetRichEditCtrl(), nFormat);

      // check for '\0's in the file. Such a character will cause the spell
      // checker not to work and may cause other issues as well.
      CString text;
      const long textLen = this->GetRichEditCtrl().GetTextLength();
      this->GetRichEditCtrl().GetTextRange(0, textLen, text);
      if (text.GetLength() != ::lstrlen(text))
      {  // invalid characters - notify the user and remove the characters
         ::AfxMessageBox(rStrInvalidCharacters, MB_OK | MB_ICONWARNING);
         if (text.Replace(_T('\0'), _T(' ')) > 0)
         {
            this->GetRichEditCtrl().SetSel(0, -1);    // select all
            this->GetRichEditCtrl().ReplaceSel(text); // replace all
            this->GetRichEditCtrl().SetSel(0, 0);     // go back to beginning
         }
      }

      this->Invalidate();

      fTextLength = Utils::GetTextLenth(this->GetRichEditCtrl().m_hWnd);
   	this->GetRichEditCtrl().GetSel(fSelectedText);
	}
}
// }end MFC

void AMswView::OnSetBookmark()
{
   if (fRtfHelper.SetBookmark())
      this->GetDocument()->SetModifiedFlag(TRUE);
}

void AMswView::OnUpdateSetBookmark(CCmdUI *pCmdUI)
{
   fRtfHelper.UpdateSetBookmark(pCmdUI);
}

void AMswView::OnGotoBookmark(UINT uId)
{
   fRtfHelper.GotoBookmark(uId);
   CMDIFrameWnd* frame = (CMDIFrameWnd*)::AfxGetMainWnd();
   CView* activeView = frame->MDIGetActive()->GetActiveView();
   if (this != activeView)
      frame->MDIActivate(this->GetParent());
}

void AMswView::OnStyle(UINT uId)
{
   const size_t i = (uId - rCmdStyle1);
   ASSERT((i >= 0) && (i < AStyle::kMaxCount));
   if (i < gStyles.size())
   {
      AMainFrame* frame = dynamic_cast<AMainFrame*>(::AfxGetMainWnd());
      if (NULL != frame)
         frame->m_wndFormatBar.SetCharFormat(gStyles[i].fFormat);
   }
}

void AMswView::OnClearBookmarks()
{
   if (IDYES == ::AfxMessageBox(rStrConfirmClear, MB_YESNO))
      if (fRtfHelper.ClearBookmarks())
         this->GetDocument()->SetModifiedFlag(TRUE);
}

void AMswView::OnUpdateClearBookmarks(CCmdUI *pCmdUI)
{
   fRtfHelper.UpdateClearBookmarks(pCmdUI);
}

void AMswView::OnNextBookmark()
{
   fRtfHelper.NextBookmark();
}

void AMswView::OnUpdateNextBookmark(CCmdUI *pCmdUI)
{
   fRtfHelper.UpdateNextBookmark(pCmdUI);
}

void AMswView::OnPrevBookmark()
{
   fRtfHelper.PrevBookmark();
}

void AMswView::OnUpdatePrevBookmark(CCmdUI *pCmdUI)
{
   fRtfHelper.UpdatePrevBookmark(pCmdUI);
}

void AMswView::OnSetPaperclip()
{
   if (fRtfHelper.SetPaperclip())
      this->GetDocument()->SetModifiedFlag(TRUE);
}

void AMswView::OnUpdateSetPaperclip(CCmdUI *pCmdUI)
{
   fRtfHelper.UpdateSetPaperclip(pCmdUI);
}

void AMswView::OnClearPaperclips()
{
   if (fRtfHelper.ClearPaperclips())
      this->GetDocument()->SetModifiedFlag(TRUE);
}

void AMswView::OnUpdateClearPaperclips(CCmdUI *pCmdUI)
{
   fRtfHelper.UpdateClearPaperclips(pCmdUI);
}

void AMswView::OnNextPaperclip()
{
   fRtfHelper.NextPaperclip();
}

void AMswView::OnUpdateNextPaperclip(CCmdUI *pCmdUI)
{
   fRtfHelper.UpdateNextPaperclip(pCmdUI);
}

void AMswView::OnPrevPaperclip()
{
   fRtfHelper.PrevPaperclip();
}

void AMswView::OnUpdatePrevPaperclip(CCmdUI *pCmdUI)
{
   fRtfHelper.UpdatePrevPaperclip(pCmdUI);
}

void AMswView::OnGotoLine()
{
   AGotoLineDlg dlg;
   if (IDOK == dlg.DoModal())
   {
      const int line = std::max<int>(0, dlg.fLine - 1);
      int pos = this->GetRichEditCtrl().LineIndex(line);
      fRtfHelper.SetCurrentUserCharPos(pos);
      this->GetRichEditCtrl().SetSel(pos, pos);
   }
}

void AMswView::OnEnChange()
{
   if (fTextLength >= 0)
   {
      const long len = Utils::GetTextLenth(this->GetRichEditCtrl().m_hWnd);
      const long delta = len - fTextLength;
      CRichEditCtrl& ed = this->GetRichEditCtrl();;
      fRtfHelper.fBookmarks.Adjust(len, delta, fSelectedText.cpMax, ed);
      fRtfHelper.fPaperclips.Adjust(len, delta, fSelectedText.cpMax, ed);
      fRtfHelper.ValidateMarks(false);

      fTextLength = len;

      this->OnRedrawCueBar(0,0);
   }

   theApp.fScriptQueueDlg.UpdateTimes();
}

void AMswView::OnEnSelchange(NMHDR* pNMHDR, LRESULT *pResult)
{
   CRichEditView::OnSelChange(pNMHDR, pResult);
   this->GetRichEditCtrl().GetSel(fSelectedText);
   *pResult = 0;
}

void AMswView::OnPaint()
{
   fRtfHelper.Paint();
   CRichEditView::OnPaint();
}

void AMswView::OnLButtonDown(UINT nFlags, CPoint point)
{
   if (fRtfHelper.StartDrag(point))
   {
      if (0x8000 & ::GetAsyncKeyState(VK_MENU))
      {  // show the cue pointer menu
         fRtfHelper.CancelDrag();

         CMenu menu;
         VERIFY(menu.CreatePopupMenu());
         UINT cuePointers[] = {rBmpCueBar0, rBmpCueBar1, rBmpCueBar2, rBmpCueBar3, rBmpCueBar4};
         static CBitmap bitmaps[_countof(cuePointers)];
         for (int i = 0; i < _countof(cuePointers); ++i)
         {
            if (NULL == bitmaps[i].GetSafeHandle())
               VERIFY(bitmaps[i].LoadBitmap(MAKEINTRESOURCE(cuePointers[i])));
            VERIFY(menu.AppendMenu(MF_ENABLED, rCmdCuePointer0 + i, &bitmaps[i]));
         }  

         POINT p;
         ::GetCursorPos(&p);
         menu.TrackPopupMenu(TPM_CENTERALIGN | TPM_LEFTBUTTON, p.x, p.y, ::AfxGetMainWnd());
      }
   }
   else
   {
      if (this->GetSelText().IsEmpty())
         this->SetDragSelect(true);
      CRichEditView::OnLButtonDown(nFlags, point);
   }
}

void AMswView::OnLButtonUp(UINT nFlags, CPoint point)
{
   if (fRtfHelper.Dragging())
      fRtfHelper.Drop(point);
   else
   {
      CRichEditView::OnLButtonUp(nFlags, point);
      this->SetDragSelect(false);
   }
}

void AMswView::OnMouseMove(UINT nFlags, CPoint point)
{
   if (fRtfHelper.Dragging())
      fRtfHelper.Drag(point);
   else
      CRichEditView::OnMouseMove(nFlags, point);
}

void AMswView::OnSysColorChange()
{
   CRichEditView::OnSysColorChange();
	m_wndRulerBar.SendMessage(WM_SYSCOLORCHANGE);
}

void AMswView::OnInitialUpdate()
{
   CRichEditView::OnInitialUpdate();
   fRtfHelper.ValidateMarks(true);

   if (theApp.fInverseEditMode)
   {
      this->ToggleTextColor(kWhite);
      this->GetDocument()->SetModifiedFlag(FALSE);
   }

   AScript script(this);
   bool scroll = true;
   for (int i = 0; ; ++i)
   {  // check the user settings to see if this one was queued last time.
      CString key;
      key.Format(_T("%s%d"), (LPCTSTR)gStrLastFile, i);
      const CString name = theApp.GetProfileString(gStrSection, key);
      if (name.IsEmpty())
         break;
      else if (0 == name.CompareNoCase(script.GetPath()))
      {
         scroll = theApp.GetProfileInt(gStrSection, key + _T('Q'), 0) ? true : false;
         break;
      }
   }

   script.SetScroll(scroll);
   if (theApp.fScriptQueue.Add(script))
      theApp.fScriptQueueDlg.RefreshList();
}

void AMswView::OnEnVscroll()
{  // force the cuebar and ruler to be redrawn
   this->OnRedrawCueBar(0, 0);
   if (theApp.fShowRuler)
      m_wndRulerBar.Invalidate();
}

void AMswView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   if (SB_THUMBTRACK == nSBCode)
      this->OnEnVscroll();
   CRichEditView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void AMswView::OnBookmarks()
{
   if (0x8000 & ::GetAsyncKeyState(VK_MENU))
      this->OnSetBookmark();
   else if (0x8000 & ::GetAsyncKeyState(VK_CONTROL))
      this->OnClearBookmarks();
   else
   {  // display the bookmarks menu
      CMenu menu;
      VERIFY(menu.CreatePopupMenu());
      fRtfHelper.BuildBookmarkMenu(menu);

      // and display the menu
      POINT pt;
      ::GetCursorPos(&pt);
      menu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, ::AfxGetMainWnd());
   }
}

void AMswView::OnFindReplace()
{
	ASSERT_VALID(this);
	m_bFirstSearch = TRUE;
   _AFX_RICHEDIT_STATE* pEditState = ::AfxGetRichEditState();
	if (pEditState->pFindReplaceDlg != NULL)
	{
		pEditState->pFindReplaceDlg->SetActiveWindow();
		pEditState->pFindReplaceDlg->ShowWindow(SW_SHOW);
		return;
	}

   CString strFind = this->GetSelText();
	// if selection is empty or spans multiple lines use old find text
	if (strFind.IsEmpty() || (strFind.FindOneOf(_T("\n\r")) != -1))
		strFind = pEditState->strFind;
	CString strReplace = pEditState->strReplace;
   AFindReplaceDlg* dlg = new AFindReplaceDlg;
	ASSERT(dlg != NULL);
	pEditState->pFindReplaceDlg = dlg;
   this->GetRichEditCtrl().GetSelectionCharFormat(dlg->fReplaceFormat);
	DWORD dwFlags = NULL;
	if (pEditState->bNext)
		dwFlags |= FR_DOWN;
	if (pEditState->bCase)
		dwFlags |= FR_MATCHCASE;
	if (pEditState->bWord)
		dwFlags |= FR_WHOLEWORD;

   pEditState->pFindReplaceDlg->m_fr.lpTemplateName = MAKEINTRESOURCE(rDlgFindReplace); 
   pEditState->pFindReplaceDlg->m_fr.hInstance = theApp.m_hInstance;
   dwFlags |= FR_ENABLETEMPLATE;

   if (!pEditState->pFindReplaceDlg->Create(false, strFind, strReplace, dwFlags, this))
	{
		pEditState->pFindReplaceDlg = NULL;
		ASSERT_VALID(this);
		return;
	}
	ASSERT(pEditState->pFindReplaceDlg != NULL);
	pEditState->bFindOnly = false;
	pEditState->pFindReplaceDlg->SetActiveWindow();
	pEditState->pFindReplaceDlg->ShowWindow(SW_SHOW);
	ASSERT_VALID(this);
}

void AMswView::OnFormatFont()
{
   POINT pt;
   ::GetCursorPos(&pt);
   CMenu menu, *pMenu = NULL;

   if (0x8000 & ::GetAsyncKeyState(VK_MENU))
      this->SendMessage(WM_COMMAND, rCmdFormatFont);
   else if (0x8000 & ::GetAsyncKeyState(VK_CONTROL))
   {  // display style menu
      VERIFY(menu.CreatePopupMenu());
      for (size_t i = 0; i < gStyles.size(); ++i)
         VERIFY(menu.AppendMenu(MF_STRING, rCmdStyle1 + i, gStyles[i].fName));
      pMenu = &menu;
   }
   else if (0x8000 & ::GetAsyncKeyState(VK_SHIFT))
   {  // display format menu
      menu.LoadMenu(rMenuFormat);
		pMenu = menu.GetSubMenu(0);
   }
   else
   {  // display size menu
      menu.LoadMenu(rMenuSize);
		pMenu = menu.GetSubMenu(0);
   }

   if (NULL != pMenu)
		pMenu->TrackPopupMenu(TPM_RIGHTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, ::AfxGetMainWnd());
}

void AMswView::OnSpacing()
{
   CParaFormat pf;
   pf.dwMask = PFM_LINESPACING | PFM_SPACEAFTER;
   const MSG* msg = this->GetCurrentMessage();
   pf.bLineSpacingRule = 5;
   switch (msg->wParam) {
      case ID_STYLE_1SPACE:   pf.dyLineSpacing = 20;   break;
      case ID_STYLE_15SPACE:  pf.dyLineSpacing = 30; break;
      case ID_STYLE_2SPACE:   pf.dyLineSpacing = 40;   break;
   }
   VERIFY(AMswView::SetParaFormat(pf));
}

void AMswView::OnStylePlain()
{
   CCharFormat format;
   format = this->GetCharFormatSelection();
   format.dwEffects &= ~(CFE_BOLD | CFE_ITALIC | CFE_UNDERLINE | CFE_STRIKEOUT);
   AMswView::SetCharFormat(format);
}

void AMswView::OnSize()
{
   int size = 0;
   switch (this->GetCurrentMessage()->wParam)
   {
      case rCmdSize12:  size = 12; break;
      case rCmdSize18:  size = 18; break;
      case rCmdSize24:  size = 24; break;
      case rCmdSize36:  size = 36; break;
      case rCmdSize40:  size = 40; break;
      case rCmdSize48:  size = 48; break;
      case rCmdSize64:  size = 64; break;
      case rCmdSize72:  size = 72; break;
      case rCmdSize96:  size = 96; break;
      default: ASSERT(false); break;
   }
   CCharFormat format;
   format.dwMask = CFM_SIZE;
   format.yHeight = size * 20;   // in TWIPS
   this->SetCharFormat(format);
}

void AMswView::OnUpdateSize(CCmdUI *pCmdUI)
{
   CCharFormat format;
   format = AMswView::GetCharFormatSelection();
   bool check = false;
   switch (format.yHeight / 20)
   {
   case 12: check = (rCmdSize12 == pCmdUI->m_nID); break;
   case 18: check = (rCmdSize18 == pCmdUI->m_nID); break;
   case 24: check = (rCmdSize24 == pCmdUI->m_nID); break;
   case 36: check = (rCmdSize36 == pCmdUI->m_nID); break;
   case 40: check = (rCmdSize40 == pCmdUI->m_nID); break;
   case 48: check = (rCmdSize48 == pCmdUI->m_nID); break;
   case 64: check = (rCmdSize64 == pCmdUI->m_nID); break;
   case 72: check = (rCmdSize72 == pCmdUI->m_nID); break;
   case 96: check = (rCmdSize96 == pCmdUI->m_nID); break;
   }

   pCmdUI->SetCheck(check);
}

void AMswView::OnSizeOther()
{
   CCharFormat format;
   format = AMswView::GetCharFormatSelection();

   AFontSizeDlg dlg;
   dlg.fSize = format.yHeight / 20;
   if (IDOK == dlg.DoModal())
   {
      format.dwMask = CFM_SIZE;
      format.yHeight = dlg.fSize * 20;   // in TWIPS
      this->SetCharFormat(format);
   }
}

CString AMswView::GetSelText()
{
   CString text;
   long start = 0, end = 0;
   this->GetRichEditCtrl().GetSel(start, end);
   this->GetRichEditCtrl().GetTextRange(start, end, text);
   return text;
}

void AMswView::OnReplaceSel(LPCTSTR lpszFind, BOOL bNext, BOOL bCase, BOOL bWord, LPCTSTR lpszReplace)
{  // replace formatting also, if requested
   // this code copied directly from CRichEditView due to a UNICODE bug in 
   // a local function, SameAsSelected
	ASSERT_VALID(this);
	_AFX_RICHEDIT_STATE* pEditState = _afxRichEditState;
	pEditState->strFind = lpszFind;
	pEditState->strReplace = lpszReplace;
	pEditState->bCase = bCase;
	pEditState->bWord = bWord;
	pEditState->bNext = bNext;

	if (!SameAsSelected(pEditState->strFind, pEditState->bCase, pEditState->bWord))
	{
		if (!FindText(pEditState))
			TextNotFound(pEditState->strFind);
		else
			AdjustDialogPosition(pEditState->pFindReplaceDlg);
	}
   else
   {
      AFindReplaceDlg* dlg = dynamic_cast<AFindReplaceDlg*>(pEditState->pFindReplaceDlg);
      if ((NULL != dlg) && dlg->fUseReplaceFormat)
         VERIFY(this->GetRichEditCtrl().SetSelectionCharFormat(dlg->fReplaceFormat));

      GetRichEditCtrl().ReplaceSel(pEditState->strReplace);
      if (!FindText(pEditState))
		   TextNotFound(pEditState->strFind);
	   else
		   AdjustDialogPosition(pEditState->pFindReplaceDlg);
   }
   ASSERT_VALID(this);
}

void AMswView::OnReplaceAll(LPCTSTR lpszFind, LPCTSTR lpszReplace, BOOL bCase, BOOL bWord)
{  // replace formatting also, if requested
   // this code copied directly from CRichEditView due to a UNICODE bug in 
   // a local function, SameAsSelected
	ASSERT_VALID(this);
	_AFX_RICHEDIT_STATE* pEditState = _afxRichEditState;
	pEditState->strFind = lpszFind;
	pEditState->strReplace = lpszReplace;
	pEditState->bCase = bCase;
	pEditState->bWord = bWord;
	pEditState->bNext = TRUE;

	CWaitCursor wait;
	// no selection or different than what looking for
	if (!SameAsSelected(pEditState->strFind, pEditState->bCase, pEditState->bWord))
	{
		if (!FindText(pEditState))
		{
			TextNotFound(pEditState->strFind);
			return;
		}
	}

	GetRichEditCtrl().HideSelection(TRUE, FALSE);

   AFindReplaceDlg* dlg = dynamic_cast<AFindReplaceDlg*>(pEditState->pFindReplaceDlg);
   int nReplacements = 0;
	do
	{
      if ((NULL != dlg) && dlg->fUseReplaceFormat)
         VERIFY(this->GetRichEditCtrl().SetSelectionCharFormat(dlg->fReplaceFormat));
		GetRichEditCtrl().ReplaceSel(pEditState->strReplace);
      ++nReplacements;
	} while (FindTextSimple(pEditState));

	m_bFirstSearch = TRUE;
   CString msg;
   msg.FormatMessage(rStrFinishedReplaceAll, nReplacements);
   AfxMessageBox(msg, MB_OK|MB_ICONINFORMATION);

   GetRichEditCtrl().HideSelection(FALSE, FALSE);

	ASSERT_VALID(this);
}

BOOL AMswView::SameAsSelected(LPCTSTR lpszCompare, BOOL bCase, BOOL /*bWord*/)
{
	// check length first
	size_t nLen = lstrlen(lpszCompare);
	long lStartChar, lEndChar;
	this->GetRichEditCtrl().GetSel(lStartChar, lEndChar);
	if (nLen != (size_t)(lEndChar - lStartChar))
		return FALSE;

	// length is the same, check contents
	CString strSelect = this->GetSelText();
	return (bCase && lstrcmp(lpszCompare, strSelect) == 0) ||
		(!bCase && lstrcmpi(lpszCompare, strSelect) == 0);
}

void AMswView::OnSelectAll()
{  // we don't want the document to scroll on 'Edit | Select all'.
   // so turn off redraw, get the scroll info, select all, reset
   // the scroll info, and turn redraw back on.
   this->GetRichEditCtrl().SetRedraw(FALSE);
   SCROLLINFO info;
   info.cbSize = sizeof(info);
   info.fMask = SIF_ALL;
   VERIFY(this->GetScrollInfo(SB_VERT, &info));
   CRichEditView::OnEditSelectAll();

   VERIFY(this->SetScrollInfo(SB_VERT, &info));
   this->SendMessage(WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, info.nTrackPos));
   this->GetRichEditCtrl().SetRedraw(TRUE);
   VERIFY(this->GetRichEditCtrl().RedrawWindow());
}

void AMswView::OnTimerToArt()
{
   AMainFrame* frame = dynamic_cast<AMainFrame*>(::AfxGetMainWnd());
   this->SetArt(frame->GetTimer() / 1000);
   theApp.fScriptQueueDlg.UpdateTimes();
}

LONG AMswView::OnSetScrollMargins(UINT, LONG)
{
   CRect client;
   this->GetClientRect(client);
   this->OnSize(SIZE_RESTORED, client.Width(), client.Height());
   return 0;
}

LONG AMswView::OnRedrawCueBar(UINT, LONG)
{
   this->InvalidateRect(fRtfHelper.GetCueBarRect());
   return 0;
}

void AMswView::OnScrollLineUp()
{
   this->GetRichEditCtrl().SendMessage(EM_LINESCROLL, 0, -1);
}

void AMswView::OnScrollLineDown()
{
   this->GetRichEditCtrl().SendMessage(EM_LINESCROLL, 0, 1);
}

void AMswView::ToggleTextColor(COLORREF newColor)
{  // toggle BLACK / WHITE text
   CRichEditCtrl& ed = this->GetRichEditCtrl();
   ed.SetRedraw(FALSE);

   // save the current selection and scroll position
   long start=0, end=0;
   ed.GetSel(start, end);
   SCROLLINFO info;
   info.cbSize = sizeof(info);
   info.fMask = SIF_ALL;
   VERIFY(this->GetScrollInfo(SB_VERT, &info));

   // convert white to black or black to white
   CRtfMemFile::SetTextColor(ed, newColor, false, false);

   // now restore the selection
   ed.SetSel(start, end);

   // and set the default font color
   CCharFormat cf;
   cf.dwMask = CFM_COLOR;
   cf.crTextColor = newColor;
   this->GetRichEditCtrl().SetDefaultCharFormat(cf);
   if (start == end)
      this->GetRichEditCtrl().SetSelectionCharFormat(cf);

   VERIFY(this->SetScrollInfo(SB_VERT, &info));
   this->SendMessage(WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, info.nTrackPos));
   ed.SetRedraw(TRUE);
   VERIFY(ed.RedrawWindow());
}

LONG AMswView::OnInverseEditMode(UINT, LONG)
{
   const BOOL modified = this->GetDocument()->IsModified();
   this->ToggleTextColor(theApp.fInverseEditMode ? kWhite : kBlack);
   this->GetDocument()->SetModifiedFlag(modified);
   return 0;
}

void AMswView::OnDropFiles(HDROP hDropInfo)
{
   dynamic_cast<AMainFrame*>(theApp.m_pMainWnd)->OnDropFiles(hDropInfo);
}

void AMswView::OnUpdatePaste(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(this->CanPaste());
}

void AMswView::OnEditPaste()
{  // if any pasted text is the same color as the background, invert it
   // first, figure out how many characters are being added and which
   // color to look for
   CRichEditCtrl& e = GetRichEditCtrl();
   long oldSelStart=0, oldSelEnd=0;
   e.GetSel(oldSelStart, oldSelEnd);
   ASSERT(oldSelEnd >= oldSelStart);
   const long oldLen = Utils::GetTextLenth(e.m_hWnd) - (oldSelEnd - oldSelStart);
   const COLORREF oldColor = (theApp.fInverseEditMode ? kBlack : kWhite);

   // do the paste
   CRichEditView::OnEditPaste();

   // now figure out how many characters were added and switch the
   // color if necessary. Note that when multiple colors are used in
   // one piece of pasted text, CFM_COLOR will not be set and we
   // won't do anything. This is probably wrong, but the fix for
   // such a case would be a mess.
   const long newLen = Utils::GetTextLenth(e.m_hWnd);
   ASSERT(newLen > oldLen);
   const long newSelEnd = oldSelStart + (newLen - oldLen);
   CCharFormat oldFormat;
   e.SetSel(oldSelStart, newSelEnd);
   VERIFY(e.GetSelectionCharFormat(oldFormat));
   if ((oldFormat.dwMask & CFM_COLOR) && 
       (oldColor == oldFormat.crTextColor))
   {  // get the pasted chars and flip the color
      CCharFormat newFormat;
      newFormat.dwMask = CFM_COLOR;
      newFormat.crTextColor = (theApp.fInverseEditMode ? kWhite : kBlack);
      e.SetSelectionCharFormat(newFormat);
   }
   e.SetSel(newSelEnd, newSelEnd);
}

void AMswView::OnEditPasteSpecial()
{
	COlePasteSpecialDialog dlg(PSF_SELECTPASTE | PSF_DISABLEDISPLAYASICON | PSF_HIDECHANGEICON);
	dlg.AddFormat(cfRTF, TYMED_HGLOBAL, AFX_IDS_RTF_FORMAT, FALSE, FALSE);
	dlg.AddFormat(CF_TEXT, TYMED_HGLOBAL, AFX_IDS_TEXT_FORMAT, FALSE, FALSE);
	if (dlg.DoModal() != IDOK)
		return;

	CWaitCursor wait;
	SetCapture();

	// we set the target type so that QueryAcceptData know what to paste
	DVASPECT dv = dlg.GetDrawAspect();
	HMETAFILE hMF = (HMETAFILE)dlg.GetIconicMetafile();
	CLIPFORMAT cf = dlg.m_ps.arrPasteEntries[dlg.m_ps.nSelectedIndex].fmtetc.cfFormat;
	m_nPasteType = dlg.GetSelectionType();
	GetRichEditCtrl().PasteSpecial(cf, dv, hMF);
	m_nPasteType = 0;

	ReleaseCapture();
}

BOOL AMswView::CanPaste() const
{
   return (CountClipboardFormats() > 0) && 
      (IsClipboardFormatAvailable(CF_TEXT) || 
       IsClipboardFormatAvailable(cfRTF));
}

void AMswView::SetDragSelect(bool selecting)
{
   if (fDragSelecting == selecting)
      return;  // nothing to do

   fDragSelecting = selecting;
   this->KillTimer(fAutoScrollTimer);
   if (fDragSelecting)
   {  // when drag selecting, the rich edit control is erratic and only
      // scrolls on mouse moves. So instead, disable this 'auto-scroll' 
      // behavior and do it ourselves.
      fSelStart = -1;
     	this->GetRichEditCtrl().SetOptions(ECOOP_AND, ~(DWORD)ECO_AUTOVSCROLL);
      fAutoScrollTimer = this->SetTimer(2, kAutoScrollFrequency, NULL);
      ASSERT(fAutoScrollTimer != 0);
   }
   else
   {
     	this->GetRichEditCtrl().SetOptions(ECOOP_OR, ECO_AUTOVSCROLL);
   }
}

void AMswView::OnUpdateColor(CCmdUI *pCmdUI)
{  // for some reason, this isn't disabled when previewing, so fix that
   bool enable = !this->m_bInPrint;
   pCmdUI->Enable(enable);
   if (enable)
   {
      CCharFormat cf = this->GetCharFormatSelection();
      enable = (cf.crTextColor == AColorMenu::GetColor(pCmdUI->m_nID));
      pCmdUI->SetCheck(enable);
   }
}
