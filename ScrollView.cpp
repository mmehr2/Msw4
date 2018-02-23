// Copyright © 2004 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"

#include "ScrollView.h"

#include <stdlib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static LPCTSTR kWindowClass         = _T("MswScroller");
static LPCTSTR kScrollViewPlacement = _T("ScrollView");

static const short kMaxWidth        = 800;
static const short kMaxHeight       = 600;
static const short kPixelDepth      = 8;
static const double kUpdateFreq     = 0.01;   // seconds
static const CRect  kMargins        = CRect(64, 10, 10, 10);

AScrollView gScrollView;
HBITMAP gImage = NULL;


static const char sDeltaPixels[] = {
    0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    3, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 
    6, 6, 6, 6, 6, 6, 6, 6, 6,10,10,10,10,10,13,13,
   13,13,13,16,16,16,16,16,20,20,20,20,20,20,20,20};

static const int sIntervalTable[] = {
   11,11,11,11,11, 8, 8, 8, 8, 8, 6, 6, 6, 6, 6, 5,
    5, 5, 5, 5, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4,
    4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

static const int  kMinSpeed         = -(mCountOf(sDeltaPixels) - 1);
static const int  kMaxSpeed         =  (mCountOf(sDeltaPixels) - 1);
static const char kSpeedBigDelta    = 21;
static const char kSpeedSmallDelta  = 11;
static const char kPausedSpeed      = 0;

//-----------------------------------------------------------------------
// Output of DirectDraw errors during debugging
#ifdef qDebug
inline void mDdErr(LPCTSTR msg, HRESULT hr)
{
   // get error message from DirectX
   CString text = CString(msg) + DXGetErrorString8(hr);
   // don't display to the user
   errors::AErrorManager::Instance().Report(NULL, (LPCTSTR)text,
    errors::kError, errors::kStream | errors::kFile | errors::kSpeaker);
}
#else
#  define mDdErr(msg, hr)
#endif


BEGIN_MESSAGE_MAP(AScrollView, CWnd)
   //{{AFX_MSG_MAP(AScrollView)
	ON_WM_PAINT()
	ON_WM_CLOSE()
	ON_WM_GETMINMAXINFO()
	ON_WM_SETCURSOR()
	ON_WM_SYSCOMMAND()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_DESTROY()
//	ON_COMMAND(rCmdPause, OnPause)
	ON_COMMAND(rCmdToggleScrollMode, OnToggleScrollMode)
//	ON_COMMAND(rCmdDecreaseSpeed, OnChangeSpeed)
//	ON_COMMAND(rCmdIncreaseSpeed, OnChangeSpeed)
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------
// class AScrollView
// Initialization / Cleanup
static ATOM sRegistration = NULL;

AScrollView::AScrollView() :
   fAccellerators(NULL),
//   fSurface(NULL),
   fFullScreen(false),
   fScrolling(false),
   fScrollPos(kMargins.left, kMargins.top),
   fSavedSpeed(kPausedSpeed),
   fEditView(NULL)
{
   this->SetSpeed(kMaxSpeed);
}


bool AScrollView::Initialize(CWnd* editView)
{
   fEditView = editView;
   if (NULL != this->m_hWnd)
   {  // the view already exists - bring it up
      this->BringWindowToTop();
      this->SetFocus();
      return true;
   }

   if (NULL == sRegistration)
   {  // register our scroll window class
      WNDCLASS w;
      ::ZeroMemory(&w, sizeof(w));
      w.style = CS_VREDRAW | CS_HREDRAW;
      w.lpfnWndProc = AfxWndProc;
      w.hIcon = ::AfxGetApp()->GetMainWnd()->GetIcon(TRUE);
      w.lpszClassName = kWindowClass;
      sRegistration = ::AfxRegisterClass(&w);
   }

   // create our window
   CString title;
   ::AfxGetApp()->GetMainWnd()->GetWindowText(title);
   if (!this->CreateEx(0, kWindowClass, title, (fFullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW),
    CW_USEDEFAULT, CW_USEDEFAULT, kMaxWidth, kMaxHeight, NULL, 0))
      return false;

   // set accellerators
//   fAccellerators = ::LoadAccelerators(::AfxGetInstanceHandle(), MAKEINTRESOURCE(rAccelScroll));

//   if (!fFullScreen)
//      AWindowPlacement::Load(this, kScrollViewPlacement);

   // initialize DirectDraw
   HRESULT hr = S_OK;
//   if ((fFullScreen && FAILED(hr = fDisplay.CreateFullScreenDisplay(this->m_hWnd, kMaxWidth, kMaxHeight, kPixelDepth))) ||
//      (!fFullScreen && FAILED(hr = fDisplay.CreateWindowedDisplay(this->m_hWnd, fMetrics.fClient.Width(), fMetrics.fClient.Height()))))
//      mDdErr(_T("CreateWindowedDisplay:"), hr);
//   else if (FAILED(hr = fDisplay.CreateSurface(&fSurface, fMetrics.fClient.Width(), fMetrics.fClient.Height())))
//      mDdErr(_T("CreateSurface:"), hr);
//   else
   {
//      mAssert(NULL == gImage);
//      editView->DrawSectionToSurface(fSurface, fMetrics.fClient);
   }

   if (SUCCEEDED(hr))
   {
      this->ShowWindow(SW_SHOW);
      this->UpdateWindow();

      this->Scroll();
   }
   else
      this->Uninitialize();
      

   return SUCCEEDED(hr);
}


bool AScrollView::Uninitialize()
{
   fScrolling = false;

   // destroy the DirectDraw objects
//   mSafeDelete(fSurface);
   HRESULT hr = S_OK;//fDisplay.DestroyObjects();

   ::DeleteObject(gImage);

   this->DestroyWindow();
   this->m_hWnd = NULL;

   //!!! restore accelerators to main wnd

   return SUCCEEDED(hr);
}


bool AScrollView::Scroll()
{
   // Initialize the counter.
//   ::DXUtil_Timer(TIMER_RESET);
   float nextUpdateTime = 0;

   // main message loop
   MSG msg;
   fScrolling = true;
   while (fScrolling)
   {
      // check demo mode
      // demo scroll time is over
      //break;
      // time to display demo message

      // check state of hand controller
      // handle button press or mouse click

      // get speed and frame rate information

      // erase frame rate info, if necessary

      // while (more frames to display)

         // while (killing time until time to display next frame)
            if (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
            {
               ::TranslateAccelerator(this->m_hWnd, fAccellerators, &msg);
               if (::GetMessage(&msg, NULL, 0, 0))
               {
                  ::TranslateMessage(&msg); 
                  ::DispatchMessage(&msg); 
               }
               else
                  fScrolling = false;
            }

         // display next frame (scroll)

      // update the timer readout
      // update script position indicator

      const DWORD now = ::GetTickCount();
/*
      const float now = ::DXUtil_Timer(TIMER_GETAPPTIME);
      if (fScrolling && (now >= nextUpdateTime))
      {  // next frame
         int delta = sDeltaPixels[::abs(fSpeed)];
         if (fSpeed < 0)
            delta = -delta;
         int newScrollPos = fScrollPos.y + delta;
         if (newScrollPos > (fMetrics.fClient.bottom - kMargins.bottom))
            newScrollPos = kMargins.top;
         else if (newScrollPos < kMargins.top)
            newScrollPos = fMetrics.fClient.bottom - kMargins.bottom;

         if (newScrollPos != fScrollPos.y)
         {
            HRESULT hr = S_OK;
            // scroll and validate the window...
//            CRect r;
//            int scrollPixels = 4;
//            this->ScrollWindow(0, -scrollPixels, &fMetrics.fClient, NULL); 
//            this->GetUpdateRect(&r);
//            this->ValidateRect(r);   

            fScrollPos.y = newScrollPos;
//            HRESULT hr = this->ProcessNextFrame();
            nextUpdateTime = now + kUpdateFreq;

            // draw the next segment
            fEditView->DrawSectionToSurface(fSurface,
             fMetrics.fClient.Width(), fMetrics.fClient.Height(), 0, 10, 0);
            CRect r = fMetrics.fClient;
            r.right -= fScrollPos.x;
            r.bottom -= fScrollPos.y;
            fDisplay.Clear(0);
            if (FAILED(hr = fDisplay.Blt(fScrollPos.x, fScrollPos.y, fSurface, r)))
               mDdErr(_T("Blt:"), hr);
            if (FAILED(hr = fDisplay.Present()))
               mDdErr(_T("Present:"), hr);
         }
      }
*/

//      const float now = ::DXUtil_Timer(TIMER_GETAPPTIME);
      int i = 0;
//      while (fScrolling && (::DXUtil_Timer(TIMER_GETAPPTIME) < (now + 1.0)))
      while (fScrolling && (::GetTickCount() < (now + 1000)))
      {
         CDC* dc = this->GetDC();
         CBrush whiteBrush(RGB(255,255,255));
         FORMATRANGE range;
         range.hdc = dc->GetSafeHdc();
         range.hdcTarget = dc->GetSafeHdc();
         range.rcPage = fMetrics.fClient;
         range.chrg.cpMin = 0;
         range.chrg.cpMax = 1000;

         int delta = 1;
         if (fSpeed < 0)
            delta = -delta;
         int newScrollPos = fScrollPos.y + delta;
         if (newScrollPos > (fMetrics.fClient.bottom - kMargins.bottom))
         {
            newScrollPos = kMargins.top;
            range.rc = fMetrics.fClient;
            dc->FillRect(&range.rc, &whiteBrush);

            range.rc.right *= 1440 / ::GetDeviceCaps(range.hdc, LOGPIXELSX);
            range.rc.bottom *= 1440 / ::GetDeviceCaps(range.hdc, LOGPIXELSY);
            ((CRichEditCtrl*)fEditView)->FormatRange(&range, true);
            ((CRichEditCtrl*)fEditView)->DisplayBand(&range.rc);
         }
         else if (newScrollPos < kMargins.top)
            newScrollPos = fMetrics.fClient.bottom - kMargins.bottom;

         HRESULT hr = S_OK;
         fScrollPos.y = newScrollPos;
         this->ScrollWindow(0, delta, &fMetrics.fClient, NULL); 

         CRect r = fMetrics.fClient;
         r = fMetrics.fClient;
         r.right -= fScrollPos.x;
         r.bottom -= fScrollPos.y;
         dc->FillRect(r, &whiteBrush);
         this->ReleaseDC(dc);
//         fEditView->DrawSectionToSurface(fSurface, r);
//         fDisplay.Clear(0);
//         if (FAILED(hr = fDisplay.Blt(fScrollPos.x, fScrollPos.y, fSurface, r)))
//            mDdErr(_T("Blt:"), hr);
//         if (FAILED(hr = fDisplay.Present()))
//            mDdErr(_T("Present:"), hr);

         ++i;
      }

      char s[16];
      ::sprintf(s, "%d frames\n", i);
      ::OutputDebugString(s);
   }

   return true;
}


//-----------------------------------------------------------------------
// Command handlers
void AScrollView::OnPause() 
{
   if (kPausedSpeed == fSpeed)
      this->SetSpeed(fSavedSpeed);
   else    
   {
      fSavedSpeed = fSpeed;
      this->SetSpeed(kPausedSpeed);               
   }
}


void AScrollView::OnToggleScrollMode() 
{
   this->Uninitialize();
}


void AScrollView::SetSpeed(int s)
{
//   fSpeed = mMin(s, kMaxSpeed);
//   fSpeed = mMax(fSpeed, kMinSpeed);
   if (fScrolling)   // set the cursor location to reflect the new speed
      ::SetCursorPos(fMetrics.fWindow.left, 
       fMetrics.fWindow.CenterPoint().y - fSpeed);
}     


//-----------------------------------------------------------------------
// Rendering
void AScrollView::OnPaint() 
{  // avoid any clipping and erasing done by GDI

   this->ValidateRect(NULL);
/*
   this->DisplayFrame();
   CWnd::OnPaint();
*/
}


HRESULT AScrollView::ProcessNextFrame()
{
   // Display the text on the screen
   HRESULT hr;
   if (FAILED(hr = this->DisplayFrame()))
   {
//      if (hr != DDERR_SURFACELOST)
      {
         mDdErr(_T("DisplayFrame:"), hr);
         return hr;
      }

      // The surfaces were lost so restore them 
      this->RestoreSurfaces();
   }

   return S_OK;
}


HRESULT AScrollView::DisplayFrame()
{
   // Fill the back buffer with black, ignoring errors until the flip
//   fDisplay.Clear(0);

   // Blt the data onto the backbuffer, ignoring errors until the flip
   HRESULT hr = S_OK;
   CRect r = fMetrics.fClient;
   r.right -= fScrollPos.x;
   r.bottom -= fScrollPos.y;
//   if (FAILED(hr = fDisplay.Blt(fScrollPos.x, fScrollPos.y, fSurface, r)))
//      mDdErr(_T("Blt:"), hr);

   // We are in fullscreen mode, so perform a flip and return 
   // any errors like DDERR_SURFACELOST
//   if (FAILED(hr = fDisplay.Present()))
   {
      mDdErr(_T("Present:"), hr);
      return hr;
   }

   return S_OK;
}


HRESULT AScrollView::RestoreSurfaces()
{
   HRESULT hr = S_OK;
//   if (FAILED(hr = fDisplay.GetDirectDraw()->RestoreAllSurfaces()))
   {
      mDdErr(_T("RestoreAllSurfaces:"), hr);
      return hr;
   }

   // We need to release and re-load, and set the palette again to 
   // redraw the bitmap on the surface.  Otherwise, GDI will not 
   // draw the bitmap on the surface with the right palette
//   LPDIRECTDRAWPALETTE  pDDPal = NULL; 
//   if (FAILED(hr = fDisplay->CreatePaletteFromBitmap(&pDDPal, MAKEINTRESOURCE(IDB_DIRECTX))))
//      return hr;
//   if (FAILED(hr = fDisplay->SetPalette(pDDPal)))
//      return hr;
//   SAFE_RELEASE(pDDPal);

   // No need to re-create the surface, just re-draw it.
//   if (FAILED(hr = fDisplay.Blt(fScrollPos.x, fScrollPos.y, fSurface, fMetrics.fClient)))
   {
      mDdErr(_T("Blt:"), hr);
      return hr;
   }

   return hr;
}


//-----------------------------------------------------------------------
// Message processing
void AScrollView::OnClose() 
{
   this->Uninitialize();
   CWnd::OnClose();
}


void AScrollView::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
   if (!fFullScreen)
   {
      lpMMI->ptMaxSize.x = lpMMI->ptMaxTrackSize.x = kMaxWidth;
      lpMMI->ptMaxSize.y = lpMMI->ptMaxTrackSize.y = kMaxHeight;
   }
	CWnd::OnGetMinMaxInfo(lpMMI);
}


BOOL AScrollView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
/*
   if (fFullScreen)
   {  // don't show cursor
      ::SetCursor(NULL);
      return TRUE;   // Nonzero to halt further processing
   }
*/

   return CWnd::OnSetCursor(pWnd, nHitTest, message);
}


void AScrollView::OnSysCommand(UINT nID, LPARAM lParam) 
{
/*
   if (fFullScreen)
   {  // Prevent moving/sizing and power loss in fullscreen mode
      switch (nID)
      {
         case SC_MOVE:
         case SC_SIZE:
         case SC_RESTORE:
         case SC_MAXIMIZE:
         case SC_MINIMIZE:
         case SC_MONITORPOWER:
         return;   // no further processing
      }
   }
*/

   CWnd::OnSysCommand(nID, lParam);
}


void AScrollView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

   fMetrics.Update();
//   fDisplay.UpdateBounds();
}


void AScrollView::OnMove(int x, int y) 
{
	CWnd::OnMove(x, y);

   fMetrics.Update();
//   fDisplay.UpdateBounds();
}


void AScrollView::OnDestroy() 
{
//   if (!fFullScreen)
//      AWindowPlacement::Save(this, kScrollViewPlacement);

   CWnd::OnDestroy();
}



//-----------------------------------------------------------------------
// class AMetrics
void AScrollView::AMetrics::Update()
{
   fScreenSize.cx = ::GetSystemMetrics(SM_CXFULLSCREEN);
   fScreenSize.cy = ::GetSystemMetrics(SM_CYFULLSCREEN);

   if (::IsWindow(gScrollView.GetSafeHwnd()))
   {
      gScrollView.GetClientRect(fWindow);
      gScrollView.ClientToScreen(fWindow);
      gScrollView.GetClientRect(fClient);
   }
}






/*!!!
One very important area of robustness that the code samples above ignored is what 
happens if the user switches away from a fullscreen DirectDraw app to some other 
app running in the system. First of all, your code needs to handle the WM_ACTIVATEAPP 
message and set a flag when your app goes inactive. In the idle section of your 
PeekMessage loop, check this flag, and don't try to update your display when you're 
not active.

But the real issue is that once you switch to another app, you have left the 
DirectDraw environment and allowed GDI to become active. GDI is oblivious to 
DirectDraw, and will overwrite the areas of display memory in use by your 
DirectDraw surfaces. This means that your bitmaps get wiped out and will need to be 
reloaded. Fortunately, when you call a function that operates on a surface, such as 
Flip() or BltFast(), DirectDraw will return an error of DDERR_SURFACELOST to let you 
know if the surface was lost. So, your code has to check for this error and reload 
the bitmaps into the surfaces if this error occurs.
*/










/* SDisplay - from 16-bit code
#define BOTTOM_BORDER 16                                                 

#if MS_DEMO 
   extern COLORREF   g_pg_colors[];
#  define DEMO_TEXT_HEIGHT   192           
#  define TIMEOUT_IN_SECONDS 20
#  define DEMO_TIMEOUT    ((DWORD) 1000*(TIMEOUT_IN_SECONDS))
   short gDemoPos = 0;
   short gDemoX = 0;
   char  gDemoString[] = "CFLP"; // [0]+, [1]-, [2]+, [3]-
   COffscreen  *gDemoPix = NULL;
#endif                              

HandController handControl;

POINT mouse1 = {0};
short centerY = 0;

int fMinFrameInterval = 0;


DWORD gDialogTimer = 0;

static CBitmap *gSliderBitmap = NULL; 
static CBitmap *gSliderBlack = NULL;

MSVD_Data   *CScrollDisplay::m_virtualDriverData = NULL;    // pointer to data (always valid)
MSVD_ProcPtr   CScrollDisplay::m_vd_proc = NULL;
CScrollDisplay::CScrollDisplay()
{
   m_have_pg_dev = FALSE;
   m_opt = SDO_LINK + SDO_COLOR;

   m_vd_proc = NULL;
   m_vd_data = NULL;
   m_virtualDriverData = NULL;
   m_vd_dll = NULL;
   m_script = NULL;
   m_min_draw = 5;
   m_full_wnd = NULL;    
   fFrameRateDlg = NULL;
   
   fIndex = 0;
   
   m_erase_brush = NULL;
#if MS_DEMO
   // "decrypt"
    gDemoString[0] += 1;
    gDemoString[1] -= 1;
    gDemoString[2] += 1;
    gDemoString[3] -= 1;
#endif    
}

CScrollDisplay::~CScrollDisplay()
{
   Unload();
   
   if (gSliderBitmap != NULL)
   {
      gSliderBitmap->DeleteObject();
      delete gSliderBitmap;
   }

   if (m_have_pg_dev)
   {
      m_have_pg_dev = FALSE;
   }
   if (m_vd_data != NULL)
   {
      GlobalUnlock(m_vd_data);
      VERIFY (NULL == GlobalFree(m_vd_data));
      m_vd_data = NULL;
      m_virtualDriverData = NULL;
   }
#if MS_DEMO
   if (gDemoPix)
      delete gDemoPix;
#endif
}     

void CScrollDisplay::Unload()
{
   // if a driver was already loaded, shut it down
   if (m_vd_proc != NULL)
   {
      DispatchVDriver(vdShutdown);
      
      if (m_vd_dll != NULL)
      {
         FreeLibrary((HINSTANCE) m_vd_dll);
         m_vd_dll = NULL;
      }
      if (m_have_pg_dev)
      {
         pgCloseDevice(&g_paige_globals, &m_pg_dev);
         m_have_pg_dev = FALSE;
      }
      
      m_vd_proc = NULL;
   }
   m_osb.Deallocate();  
}

BOOL CScrollDisplay::Load(LPSTR driverName)
{   
   RECT r;

   if (m_vd_data == NULL)
   {
      m_vd_data = GlobalAlloc(GHND, sizeof(MSVD_Data));
      ASSERT(m_vd_data != NULL);
      m_virtualDriverData = (MSVD_Data *) GlobalLock(m_vd_data);
      m_virtualDriverData->dirty = TRUE;
   }

   if (gSliderBitmap == NULL)
   {
      ASSERT (NULL == gSliderBitmap);
      gSliderBitmap = new CBitmap;
      gSliderBitmap->LoadBitmap(IDB_SLIDER);
   }

   
   Unload();
   
   m_virtualDriverData->sdo_flags = m_opt;   // set current options for loading driver

   if (driverName == NULL)    // default driver
   {     // set default output display parameters
      if (m_vd_proc == NULL)
         m_vd_proc = (MSVD_ProcPtr) ::VDMainScreen;

      // set color info from main displays
      m_virtualDriverData->phys_planes = 1;  // default to B&W
      m_virtualDriverData->phys_depth = 1;
      if ((m_opt & SDO_COLOR) != 0)
      {
         HDC dc = CreateDC("DISPLAY", NULL, NULL, NULL);
         if (dc != NULL)
         {
            m_virtualDriverData->phys_planes = GetDeviceCaps(dc, PLANES);
            m_virtualDriverData->phys_depth = GetDeviceCaps(dc, BITSPIXEL);
            DeleteDC(dc);
         }
      }
      DispatchVDriver(vdStartup);

      int depth = m_virtualDriverData->phys_depth;
      m_osb.Allocate(m_virtualDriverData->phys_width, m_virtualDriverData->phys_height * 4,
            m_virtualDriverData->phys_planes, depth);
      
   }

   // create the paige device for the offscreen
   pgInitDevice(&g_paige_globals, MEM_NULL, (generic_var)m_osb.GetHDC(), &m_pg_dev);
   m_have_pg_dev = TRUE;
   
   // make default visible area cover entire display
   m_osb.GetArea(&r);
   m_vis_rect.top_left.h = r.left; m_vis_rect.top_left.v = r.top;
   m_vis_rect.bot_right.h = r.right; m_vis_rect.bot_right.v = r.bottom;
   m_vis = pgRectToShape(g_mem_globals, &m_vis_rect);

   {
      CBrush br(RGB(0x00, 0x00, 0x00));
      m_osb.Select()->FillRect(&r, &br);
      m_osb.Deselect();
   }

   // set up drawing variables
   m_virtualDriverData->end_line = m_osb.GetHeight();
   m_max_draw = m_virtualDriverData->end_line - (m_virtualDriverData->phys_height);// << 1);
   Invalidate();
   
#if MS_DEMO
if (gDemoPix != NULL)
   delete gDemoPix;

{  RECT tr; 
   CFont demoFont, *saveFont;    
   short saveMode;
   COLORREF saveColor;  
   CSize textSize;
   CDC dc, *pCDC; 
   
   ASSERT (NULL == gDemoPix);
   gDemoPix = new COffscreen;    

   // use temp DC for text dimensions
   dc.CreateCompatibleDC(NULL);
   pCDC = &dc;

   // create font
   demoFont.CreateFont(DEMO_TEXT_HEIGHT, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
      ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
      DEFAULT_QUALITY, TMPF_TRUETYPE | VARIABLE_PITCH | FF_SWISS, NULL);

   // measure font
   saveFont = pCDC->SelectObject(&demoFont);
   textSize = pCDC->GetTextExtent(gDemoString, sizeof(gDemoString)-1);
   pCDC->SelectObject(saveFont);             

   textSize.cx += 40; textSize.cy += 40;
   gDemoPix->Allocate(textSize.cx, textSize.cy, 
            m_virtualDriverData->phys_planes, m_virtualDriverData->phys_depth);

   gDemoPix->GetArea(&tr);       
   
   // draw "DEMO"
   pCDC = gDemoPix->Select(); 
   CBrush br(RGB(0x0, 0x0, 0x77));
   pCDC->FillRect(&tr, &br);
   saveMode = pCDC->SetBkMode(TRANSPARENT);   
   if (m_virtualDriverData->phys_depth > 1 || m_virtualDriverData->phys_planes > 1)
      saveColor = pCDC->SetTextColor(RGB(0xFF, 0x00, 0x00));
   else
      saveColor = pCDC->SetTextColor(RGB(0xFF, 0xFF, 0xFF));
   saveFont = pCDC->SelectObject(&demoFont);
   pCDC->SetTextColor(RGB(0xFF, 0xFF, 0xFF));
   pCDC->TextOut(tr.left + 30, tr.top + 20, gDemoString, strlen(gDemoString));
   pCDC->SelectObject(saveFont);             
   pCDC->SetTextColor(saveColor);  
   pCDC->SetBkMode(saveMode);        

   gDemoPix->Deselect();
}
#endif
   return TRUE;
}

void CScrollDisplay::GetCurrentScrollPos(CScriptView **pView, long *pCharPos, long *pPixelPos)
{
   CScrollPos spos = m_pos;
   spos.Offset(CCueBar::m_cue_pos);

   long pix = spos.GetCurPos();
   if (fLastDirection >= 0)   // backward
      pix -= m_full_wnd->GetHeight();
      
   if (pPixelPos != NULL)
      *pPixelPos = pix;

   CScriptView *sv = spos.GetCurScript();
   if (pView != NULL)
      *pView = sv;
   
   if (sv != NULL && pCharPos != NULL)
   {
      pgSetScrollValues (sv->GetPgRef(), 0, 0, FALSE, draw_none);
      co_ordinate pt = {0,0};
      pt.v = pix - sv->m_start;
      *pCharPos = pgPtToChar(sv->GetPgRef(), &pt, NULL);
   }
}

void CScrollDisplay::JumpToCharPos(CDC* pDC, long nPos, BOOL bStopScroll)
{
   CScriptView *sv;
   rectangle r;
   CScrollPos spos;
   
   spos = m_pos;
   spos.Offset(CCueBar::m_cue_pos);
   sv = spos.GetCurScript();
   if (sv != NULL)
   {
      pgCharacterRect(sv->GetPgRef(), nPos, FALSE, FALSE, &r);
      m_pos.SeekTo(r.top_left.v + sv->m_start - CCueBar::m_cue_pos);
   }
   if (bStopScroll)
   {
      SetSpeed(0);
      fDeltaPos = 0;
   }
   Invalidate(); 
   
   this->DrawWholeScreen(pDC, m_full_wnd->GetHeight() - BOTTOM_BORDER, 1);   
}

void CScrollDisplay::Invalidate(void)
{
   m_line = 0;
   if (m_virtualDriverData)
   {
      m_virtualDriverData->drawn = 0;
      m_virtualDriverData->top_line = 0;
      m_virtualDriverData->draw_top = -1;
   }
}

void CScrollDisplay::SetFrameRate(WORD nFPS)
{ 
}

void CScrollDisplay::SetScript(CScriptView *pScript)
{
   m_script = pScript;
   Invalidate();
}


DWORD CScrollDisplay::DispatchVDriver(WORD wMessage)
{
   if (m_vd_proc != NULL)
      return (*m_vd_proc)(m_virtualDriverData, wMessage);
   else
      return -666;   // no driver!
}

static short gDebugViewScope = 0;

void CScrollDisplay::DrawTimer()
{
   CControlPalette *cp;

   cp = &AMainFrame::m_cp;
   if (m_full_wnd != NULL)
   {
      CClientDC fulldc(m_full_wnd);
        cp->DrawTimer(&fulldc, 8, 8);
   } else
      cp->DrawTimer();
}

void CScrollDisplay::SetCopyRangeAll()
{
   m_virtualDriverData->copy_left = 0;
   m_virtualDriverData->copy_right = GetWidth();
}

void CScrollDisplay::InitScrollView(BOOL useQueue)
{
   CScriptQueue *q = AMainFrame::GetQueuePtr();
   RECT r;
   
   m_virtualDriverData->script_left = m_virtualDriverData->scroll_left + CScriptRuler::GetLeftMargin();
   m_virtualDriverData->script_right = m_virtualDriverData->scroll_left + CScriptRuler::GetRightMargin();

#if MS_DEMO
   gDemoPos = 0;
   gDemoX = ((m_virtualDriverData->script_right - m_virtualDriverData->script_left) - gDemoPix->GetWidth()) >> 1;
   gDemoX += m_virtualDriverData->script_left;
   if (gDemoX < 0) gDemoX = 0;
#endif      
                                
   m_pause_speed = 0;                                
   fScrolling = TRUE;
   m_virtualDriverData->sdo_flags = m_opt;
   
   // set up scrolling bounds
   if ((m_opt & SDO_LOOP) == 0)  // not looping: let scroll back a screenfull
      m_virtualDriverData->min_pos = -m_virtualDriverData->phys_height;
   else
      m_virtualDriverData->min_pos = 0;   // looping: top is at beginning

   ASSERT (NULL == m_erase_brush);
   m_erase_brush = new CBrush;
   if ((m_opt & SDO_INVERSE) != 0)
      m_erase_brush->CreateSolidBrush(RGB(0x00, 0x00, 0x00));
   else
      m_erase_brush->CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
                                     
   CDC *osdc = m_osb.Select();
   ASSERT(osdc != NULL);
   r.left = 0; r.right = m_virtualDriverData->phys_width;   
   r.top = 0; r.bottom = m_virtualDriverData->end_line;
   osdc->FillRect(&r, m_erase_brush);  
   m_osb.Deselect();

   Invalidate();
   
   m_virtualDriverData->scroll_delta = 0;
   SetCopyRangeAll();

   // get all the linked scripts in the queue
   q->PrepForScroll(&m_pos, useQueue);    // check for error?    
   q = AMainFrame::GetQueuePtr();
   m_virtualDriverData->top_pos = m_pos.GetCurPos();
   m_virtualDriverData->max_pos = q->GetExtent();
}

void CScrollDisplay::ExitScrollView()
{
   CScriptQueue *q = AMainFrame::GetQueuePtr();

   q->ScrollDone(&m_pos);

   delete m_erase_brush;
   m_erase_brush = NULL;
   fScrolling = FALSE;
}

long gLastJumpPos = -1;
int gLastJumpIx = -1;   
int gFinished = 0;


// stays in scroll mode until mouse click or escape key hit
void CScrollDisplay::DoScrollMode()
{
   POINT saveCursorPos;
   BOOL nullMsg;
   MSG msg;
   CScriptQueue *q = AMainFrame::GetQueuePtr();
   RECT r, clip;
   CControlPalette *cp;
   CCaptionPalette *cc;                     
   HCURSOR curs, saveCurs;
   int index;
#if MS_DEMO
   DWORD entryTime = GetTickCount();
   DWORD demoMessageTime = GetTickCount();
#endif
   int newSpeed;
   BOOL switchUp = FALSE;
   ScrollPosIndicator spi;
   
                  
   // check the key
   if (!AddLongInt())
      return;
   
   SetSpeed(0);
   
   // set up drawing offscreen area
   InitScrollView(TRUE);

   // we use these often
   cp = &AMainFrame::m_cp;
   cc = &AMainFrame::m_cc;
   
   // DrawTimer   
   (AMainFrame::m_cp).SetTimerValue(0);

   // clear paperclips if prefs set or ALT key down
   if (AMainFrame::m_pref.clearPC || (GetAsyncKeyState(VK_MENU) < 0))
   {
      CScriptQueue *q = AMainFrame::GetQueuePtr();
      CScriptView *script;
      POSITION prev, pos;
   
      pos = prev = q->GetHeadPosition();  // clear for all scripts:
      while (pos != NULL)
      {
         script = q->GetNext(pos);
         (script->GetPaperclips())->ClearPaperclips();
      }
   }
   gLastJumpPos = -1;
   gLastJumpIx = -1;

   if (cc->BeginCaption(&m_pos) == FALSE)
      ::MessageBeep(0);

   // cover display with popup window (*** not if driver uses 2nd screen ***)
   if (NULL == m_full_wnd)
      m_full_wnd = new CFullScreenWnd;    

   if (m_have_pg_dev)
   {
      pgCloseDevice(&g_paige_globals, &m_pg_dev);
      m_have_pg_dev = FALSE;
   }

   ASmartDC pWindowDC(m_full_wnd, ASmartDC::kWindow);
   pgInitDevice(&g_paige_globals, MEM_NULL, (generic_var)pWindowDC->GetSafeHdc(), &m_pg_dev);
   m_have_pg_dev = TRUE;
   
   m_full_wnd->Create(AfxGetApp()->m_pMainWnd);
   
   CDC *osdc = m_osb.Select();
   ASSERT(osdc != NULL);  
   
   r.left = 0;
   r.top = 0;
   r.right = m_virtualDriverData->phys_width;
   r.bottom = m_virtualDriverData->phys_height + 100;
   
   m_osb.Select()->FillRect(&r, m_erase_brush);     
   
   r.bottom = m_virtualDriverData->phys_height;
   ASmartDC pDC(m_full_wnd);
   pDC->FillRect(&r, m_erase_brush);
   if (CCueBar::GetCueArrow() != 0)
      m_osb.DrawMaskedBitmap(CCueBar::GetCueArrowBitmap(), m_virtualDriverData->script_left - 32 - 8,
             m_virtualDriverData->top_line + CCueBar::m_cue_pos - 16, (m_opt & SDO_INVERSE) != 0); 

   r.left = 0;
   r.top = (m_virtualDriverData->phys_height>>1)-(280/2);
   r.right = r.left + 18;     
   r.bottom = r.top + 280;
   if (AMainFrame::m_pref.showSlider)
   {
      m_osb.DrawBitmap(gSliderBitmap, r.left, r.top, SRCCOPY);
      r.left = r.right; r.right += 18;
    }
   m_osb.Deselect();

   ASSERT (NULL == m_screen_dc.m_hDC);
   m_screen_dc.CreateDC("DISPLAY", NULL, NULL, NULL);

   m_virtualDriverData->script_dc = osdc->m_hDC;
   m_virtualDriverData->script_bm = (HBITMAP) (m_osb.GetBitmap())->m_hObject;
   ::GetObject(m_virtualDriverData->script_bm, sizeof(BITMAP), &m_virtualDriverData->sbm_data);

   m_virtualDriverData->sbm_data.bmBits = m_osb.GetBitsPtr();

   GlobalFix(m_virtualDriverData->script_bm);
   GlobalFix(m_vd_data);
   m_virtualDriverData->screen_dc = m_screen_dc.m_hDC;
   m_virtualDriverData->draw_time = m_virtualDriverData->current_time;
   
   DispatchVDriver(vdEnterScroll);
   
   DispatchVDriver(vdUpdateScroll); // 1st draw w/o text (?)

   if (AMainFrame::m_pref.showScriptPos)
   {
      CClientDC fulldc(m_full_wnd);
      spi.Create(&r, &fulldc);
    }
   if (AMainFrame::m_pref.showTimer)
      DrawTimer();

   fMinFrameInterval = GetPrivateProfileInt(kSection, kEntry, 0, kIniFile);
   if (0 == fMinFrameInterval)                                                  
   {
      ::MessageBox(NULL,"Use the LEFT- and RIGHT-ARROW keys to tune MagicScroll for optimum scrolling performance on your computer.  MagicScroll will remember your settings and use them in the future.", "First Time Users:", MB_OK);
      fMinFrameInterval = 27; // milliseconds per frame
   }
   

// if (flags & VDF_MAINSCREEN)
   // center the mouse on the screen for maximum movement
   GetCursorPos(&saveCursorPos);                      
   ::SetCapture(m_full_wnd->GetSafeHwnd());
   centerY = m_virtualDriverData->phys_height >> 1;
   mouse1.y = centerY;
   SetCursorPos(mouse1.x, centerY);
   curs = ::LoadCursor(AfxGetInstanceHandle(), (LPCSTR) 
      (AMainFrame::m_pref.showSlider ? IDC_SCROLL : IDC_SCROLL1));
   saveCurs = ::SetCursor(curs);

   clip.top = mouse1.y - 127;
   clip.left = clip.right = 0;
   clip.bottom = mouse1.y + 128;
   ClipCursor(&clip);

   int test = timeBeginPeriod(1);
   ASSERT(TIMERR_NOERROR == test);   
   fPrevFrameTime = this->GetCurrTime();  
 
 
   r.left = 0;
   r.top = m_virtualDriverData->phys_height - 80;
   r.right = m_virtualDriverData->phys_width;
   r.bottom = m_virtualDriverData->phys_height + 100;
   
   pDC->FillRect(&r, m_erase_brush);
 
   // draw the whole initial screen...
   this->DrawWholeScreen(pDC, (m_full_wnd->GetHeight() - BOTTOM_BORDER), 1);
   gFinished = 0;

   // clear the hand controller so that we don't detect another press right away
   DWORD lastHandControlPress = handControl.IsConnected() ? ::GetTickCount() : 0;
   handControl.Disconnect();

   ////  top of scroll loop ///////////////////////////////////////////
   TRACE("Scroll Mode\n");
   while (0 == gFinished)
   {    
   
   #if MS_DEMO
      if (GetTickCount() - entryTime > DEMO_TIMEOUT)
         break;
      if (GetTickCount() - demoMessageTime > 3000)
      {  
         demoMessageTime = GetTickCount();  
         
         m_full_wnd->DrawBitmap(gDemoPix->GetBitmap(), gDemoX + 26, 
            m_full_wnd->GetHeight() / 2, SRCINVERT); 
      }
   #endif
   
      // update hand controller           
      int cmd;
      BOOL exitFlag = FALSE; 
      long textOffset, pixelOffset;
      CScriptView *sv;

      int chg, sw;
      BOOL mouseClicked = FALSE, leftPressed = FALSE, rightPressed = FALSE; 
      handControl.UpdateController(TRUE, &mouseClicked);
      sw = handControl.GetSwitches(&chg); 
      if ((sw & hcsLeftButton) && (chg & hcsLeftButton))
         leftPressed = TRUE;
      if ((sw & hcsRightButton) && (chg & hcsRightButton))
         rightPressed = TRUE;

      if ((::GetTickCount() > (lastHandControlPress + 500)) && (leftPressed || rightPressed))
      {
         if (!mouseClicked)
            lastHandControlPress = ::GetTickCount();
            
         GetCurrentScrollPos(&sv, &textOffset, &pixelOffset);
         if (leftPressed && mouseClicked)
            cmd = HCBTN_EXIT;
         else
            cmd = (leftPressed) ? AMainFrame::m_pref.leftButton :
             AMainFrame::m_pref.rightButton;
               
         switch (cmd)
         {
            case HCBTN_EXIT:
            exitFlag = TRUE;
            break;         
            
            case HCBTN_NEXTBKMK:
            case HCBTN_PREVBKMK:
            {  
               if (sv != NULL)
               {  CBookmarks *bm;
                  int i, ix;
                  long bi, di, bmpos;
                     
                  ix = -1;
                  bi = -1;
                  di = 0x7FFFFFFF;
                  bm = sv->GetBookmarks();
                  for (i = 0; i < 20; i++)
                  {
                     bm->GetBookmark(i, NULL, &bmpos, NULL);
                     if (bmpos >= 0)
                     {
                        if (cmd == HCBTN_NEXTBKMK)
                        {
                           if (bmpos > textOffset)
                           {
                              // check for same char rect.top and skip???
                              if (bmpos - textOffset < di && (gLastJumpIx != i || textOffset != gLastJumpPos))
                              {
                                 di = bmpos - textOffset;
                                 bi = bmpos;
                                 ix = i;
                              }
                           }
                        } else {
                           if (bmpos < textOffset)
                           {
                              // check for same char rect.top and skip???
                              if (textOffset - bmpos < di && (gLastJumpIx != i || textOffset != gLastJumpPos))
                              {
                                 di = textOffset - bmpos;
                                 bi = bmpos;
                                 ix = i;
                              }
                           }
                        }
                     }
                  }
                  if (bi != -1)
                  {    
                     JumpToCharPos(pDC, bi, TRUE);
                     GetCurrentScrollPos(&sv, &gLastJumpPos, NULL); 
                     gLastJumpIx = ix;
                  }
               }
            }
            break;

            case HCBTN_SETPC:
            this->SetPaperclip();
            break;

            case HCBTN_GOTOPC:
            this->GotoPaperclip();
            break;
         }
      }
      if (exitFlag)
         break;
         
      // udpate closed captioning
      cc->UpdateCaption(&m_pos); 
      
      // begin scrolling functions...
      
      // find the next scroll position to draw...
      int numIntervalsRemaining;
      int deltaPosRemaining;
      int currentFrameInterval;     
      this->GetDesiredSpeed(deltaPosRemaining, numIntervalsRemaining);  // sets fDeltaPos & fSpeed...
      if ( fSpeed > -4 && fSpeed < 4)
      {  
         // stop that scrolling...
         fDeltaPos = 0;
         deltaPosRemaining = 0;
         numIntervalsRemaining = 1;
      }
      
      if (NULL != fFrameRateDlg)
      {  
         // leave the frame rate dialog up for a few moments...
         if (gDialogTimer < 1)
         {  
            // get rid of the dialog and redraw anything that might have been behind it
            delete fFrameRateDlg;
            fFrameRateDlg = NULL;
            gDialogTimer = 0;   
            CRect r;
            r.top = 18;
            r.bottom = 15 + 80;
            r.left = 0;
            r.right = 50;     
            pDC->FillRect(&r, m_erase_brush);      

            if (CCueBar::GetCueArrow() != 0) 
               r.left = m_virtualDriverData->script_left - 32 - 8;
               r.top =  m_virtualDriverData->top_line + CCueBar::m_cue_pos - 16;
               r.right = r.left + 36;
               r.bottom = r.top + 36;
               pDC->FillRect(&r, m_erase_brush);
//               m_full_wnd->DrawMaskedBitmap(CCueBar::GetCueArrowBitmap(), m_virtualDriverData->script_left - 32 - 8,
//               m_virtualDriverData->top_line + CCueBar::m_cue_pos - 16, (m_opt & SDO_INVERSE) != 0); 
         }
         else
            gDialogTimer--;
      }
      // Mr. Timing Loop.... 
      
      while (numIntervalsRemaining > 0)
      {  
         if (1 == numIntervalsRemaining) 
            fDeltaPos = deltaPosRemaining;
         else 
            fDeltaPos = 1; 
         
         if (1 == deltaPosRemaining)
         {
            currentFrameInterval = numIntervalsRemaining * fMinFrameInterval;
            numIntervalsRemaining = 1;
         }
         else
            currentFrameInterval = fMinFrameInterval; 
         
         numIntervalsRemaining--;
         deltaPosRemaining--;                                        
         int k = 0;                           
         while ((k < 1) || (timeGetTime() - fPrevFrameTime < currentFrameInterval))
         {
            k++;
            // check for messages while we're killing time...  
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {     
               if (msg.message == WM_QUIT)
                  break;
               else if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
               {
                  if (DoKeyDown(msg.wParam, msg.lParam) == FALSE)
                     gFinished = 1;
               }
            }
         }
         fPrevFrameTime = timeGetTime();            
                                                       
         // do the Windows scroll function...
          
         // only scroll the script portion of the screen...
         RECT sRect;
         sRect.top = 0;
         sRect.bottom = m_full_wnd->GetHeight() - BOTTOM_BORDER;
         sRect.left = CScriptRuler::GetLeftMargin()+ m_virtualDriverData->scroll_left;
         sRect.right = m_full_wnd->GetWidth();
         
         // scroll the window...
         if(fSpeed > 0) fDeltaPos = -fDeltaPos;
         m_full_wnd->ScrollWindow(0, fDeltaPos, &sRect, NULL); 
    
   
         CRect updateRect;
         m_full_wnd->GetUpdateRect(&updateRect);
         m_full_wnd->ValidateRect(updateRect);   
         
         // clean up the top and bottom of the screen
         sRect.top = 0;
         sRect.bottom = 1;
         sRect.left = 0;
         sRect.right = m_full_wnd->GetWidth();
         pDC->FillRect(&sRect, m_erase_brush); 
         
         sRect.top = m_full_wnd->GetHeight() - BOTTOM_BORDER;
         sRect.bottom = m_full_wnd->GetHeight();
         pDC->FillRect(&sRect, m_erase_brush);
         
         // fill in the blank spaces left by the scroll move...
         if (fDeltaPos < 0)
         {  
            // we're going forward so the space is at the bottom of the screen... 
            // DrawSomeRows(how many/which direction, distance from top of screen to put it);
            this->DrawSomeRows(pDC, -fDeltaPos, m_full_wnd->GetHeight() - BOTTOM_BORDER);  
         }
         else if (fDeltaPos > 0)
         {
            // we're rewinding so the space is at the top of the screen...
            this->DrawSomeRows(pDC, -fDeltaPos, 1);        
         }
         else {}  // we ain't goin' nowhere...
      }
   
      // update the timer readout
      if (cp->UpdateTimer() && AMainFrame::m_pref.showTimer)
         DrawTimer();
         
      // update script position indicator
      if (AMainFrame::m_pref.showScriptPos)
         spi.Update(m_pos.GetCurPos(), pWindowDC);
   } // while (0 == gFinished)
   
   char frame[20];
   wsprintf(frame, "%d", fMinFrameInterval);
   WritePrivateProfileString(kSection, kEntry, frame, kIniFile);
   
   /////////////////////////////////////// bottom of loop ///////////////////

   if (m_have_pg_dev)
   {
      pgCloseDevice(&g_paige_globals, &m_pg_dev);
      m_have_pg_dev = FALSE;
   }

   pgInitDevice(&g_paige_globals, MEM_NULL, (generic_var)m_osb.GetHDC(), &m_pg_dev);
   m_have_pg_dev = TRUE;

   if (NULL != fFrameRateDlg)
   {
      delete fFrameRateDlg;
      fFrameRateDlg = NULL;
   }
   
   timeEndPeriod(1);
   
   cc->EndCaption();
   ::ReleaseCapture();

   ClipCursor(NULL);
   ::SetCursor(saveCurs);
   SetCursorPos(saveCursorPos.x, saveCursorPos.y);
   DispatchVDriver(vdExitScroll);
   cp->SetTimerEnable(FALSE);

   GlobalUnfix(m_virtualDriverData->script_bm);
   GlobalUnfix(m_vd_data);
   
   m_screen_dc.DeleteDC();
   
   // scroll script to current position
   {  
      CScriptView *sv;
      long charPos;
      this->GetCurrentScrollPos(&sv, &charPos, NULL);
      if (sv != NULL)
      {  
         // if pg needs to scroll down to get to charPos, charPos will
         // end up on the bottom of the screen.  So, in order to get charPos
         // on the top of the screen, we scroll past it, then back.
         pgScrollToView(sv->GetPgRef(), LONG_MAX, 0, 0, TRUE, best_way);
         pgScrollToView(sv->GetPgRef(), charPos, 0, 0, TRUE, best_way);
         pgSetSelection(sv->GetPgRef(), charPos, charPos, 0, FALSE);
         sv->GetParentFrame()->ActivateFrame();  // bring window to top
      }
   }

   ExitScrollView();
   if (m_full_wnd != NULL)
   {
      delete m_full_wnd;
      m_full_wnd = NULL;   
   }
}


void CScrollDisplay::SetPaperclip()
{                                   
   CScriptView *sv = m_pos.GetCurScript();
   if (sv == NULL)
      return;
      
   long charPos;
   GetCurrentScrollPos(&sv, &charPos, NULL);
   CPaperclips* pc = sv->GetPaperclips();
   if (NULL == pc)
      return;
      
   pc->AddPaperclip(charPos);
   sv->Dirty();
         
   short x = m_virtualDriverData->script_left;
//   m_full_wnd->DrawMaskedBitmap(&CCueBar::m_paperclip, 
//      x, m_virtualDriverData->top_line + CCueBar::m_cue_pos - 16, FALSE); 
         
   CRect updateRect;
   m_full_wnd->GetUpdateRect(&updateRect);
   m_full_wnd->ValidateRect(updateRect);   
}


void CScrollDisplay::GotoPaperclip()
{
   CScriptView *sv = m_pos.GetCurScript();
   if (NULL == sv)
      return;
      
   CPaperclips* pc = sv->GetPaperclips();
   if (NULL == pc)
      return;

   long pos;
   if (GetAsyncKeyState(VK_SHIFT) < 0)
      pos = pc->PrevPaperclip();
   else
      pos = pc->NextPaperclip();

   if (-1 == pos)
      return;
      
   ASmartDC pDC(m_full_wnd);
   JumpToCharPos(pDC, pos, TRUE);
}


BOOL CScrollDisplay::DoKeyDown(WORD nChar, DWORD nFlags)
{
   BOOL result = TRUE;
   BOOL altKey = (nFlags & ((long) 1 << 29)) != 0;
   
   if (nChar >= '0' && nChar <= '9')
   {  CBookmarks *bm;
      CScriptView *sv;
      short bmix;
      long bmpos;
      
      bmix = nChar - '0' - 1;
      if (bmix < 0) bmix = 9;
      if (GetAsyncKeyState(VK_SHIFT) < 0)
         bmix += 10;
      
      GetCurrentScrollPos(&sv, NULL, NULL);
      if (sv != NULL)
      {
         bm = sv->GetBookmarks();
         bm->GetBookmark(bmix, NULL, &bmpos, NULL);
         if (bmpos >= 0)
         {
            ASmartDC pDC(m_full_wnd);
            JumpToCharPos(pDC, bmpos, TRUE);
         }
      }
   }
   else switch (nChar)
   {
      case VK_LEFT:
         m_full_wnd->OnSlowerFrames();
         if (NULL == fFrameRateDlg)
         { 
            ASSERT (NULL == fFrameRateDlg);
            fFrameRateDlg = new CRateDlg(m_full_wnd);
            fFrameRateDlg->Create();
         }                 
         gDialogTimer = 50;
         fFrameRateDlg->SetText(fMinFrameInterval);
         break;  
      
      case VK_UP:
         fIndex += altKey ? 21 : 11;
         this->SetSpeed(fIndex);
         
      break;

      case VK_RIGHT:
         m_full_wnd->OnFasterFrames();  
         if (NULL == fFrameRateDlg)
         { 
            ASSERT (NULL == fFrameRateDlg);
            fFrameRateDlg = new CRateDlg(m_full_wnd);
            fFrameRateDlg->Create();
         }
         gDialogTimer = 50;  
         fFrameRateDlg->SetText(fMinFrameInterval);
         
      break;

      case VK_DOWN:
         fIndex -= altKey ? 21: 11;   
         this->SetSpeed(fIndex);       
      break;

      case VK_HOME:
      break;
               
      case VK_END:
      break;
            
      case VK_PRIOR:
      break;

      case VK_F1:
         AMainFrame::m_pref.speedSensitivity = 2;
      break;

      case VK_F2:
         AMainFrame::m_pref.speedSensitivity = 1;
      break;

      case VK_F3:
         AMainFrame::m_pref.speedSensitivity = 0;
      break;

      case VK_F12:
         gDebugViewScope ^= 1;
      break;

      case VK_NEXT:
      break;
      
      case VK_RETURN:    
         if (fSpeed == 0)
            SetSpeed(m_pause_speed);
         else    
         {
            m_pause_speed = fSpeed;
            SetSpeed(0);               
         }
      break;
      
      case VK_ESCAPE:
         result = FALSE;
      break;

      case VK_SPACE:    // set paperclip
      if (GetAsyncKeyState(VK_CONTROL) < 0)
         this->GotoPaperclip();
      else
         this->SetPaperclip();
      break;

      case 't':
      case 'T':
      {  
         CControlPalette *cp;
         cp = &AMainFrame::m_cp;
         cp->SetTimerEnable(!cp->GetTimerEnable());
      }
      break;
   }
   return result;
}
 
 
void CScrollDisplay::DrawWholeScreen(CDC* pDC, int numRows, int atWhichRow)
{
   DrawPiece(pDC, &m_pos, atWhichRow, numRows);
   fLastDirection = 0;
} 
 

void CScrollDisplay::DrawSomeRows(CDC* pDC, int numRows, int atWhichRow)
{  
   int rowsToScroll;                              
   long pos = m_pos.GetCurPos();
   if (numRows < 0)  // we're rewinding...     
   {  
      // did we just switch directions, and need to get a new 
      // scroll position at the top of the screen?  
      if (fLastDirection >= 0)
      {
         fLastDirection = numRows;
         m_pos.Offset( -(m_full_wnd->GetHeight() - BOTTOM_BORDER ));       
      }
      m_pos.Offset(numRows);  
      rowsToScroll = - numRows;  
      DrawPiece(pDC, &m_pos, atWhichRow, rowsToScroll);
      m_pos.Offset(numRows);
   }
   else if (numRows > 0) 
   // we're going forward...
   {
      if (fLastDirection < 0)
      {
         fLastDirection = numRows;
         m_pos.Offset( m_full_wnd->GetHeight() - BOTTOM_BORDER);
      }
      rowsToScroll = numRows;               
      DrawPiece(pDC, &m_pos, atWhichRow - rowsToScroll, rowsToScroll);
   }  
}


int CScrollDisplay::GetDesiredSpeed(int &deltaPixels, int &frameInterval)
{
   int dy = 0;
   BOOL twistingHandControl = FALSE;
   if (handControl.IsConnected())
   {
      // set cursor position from hand control...  
      int controlY, newY;
            
      handControl.UpdateController();
      controlY = handControl.GetNewSpeed();
      static int oldSpeed = -1;
      if (oldSpeed != controlY)
      {                       
         twistingHandControl = TRUE;
         oldSpeed = controlY;
         newY = centerY - controlY;
         if (AMainFrame::m_pref.reverseSpeed)
            newY = -newY;
         if (controlY != 0xFFFF)
            SetCursorPos(fCurrMouse.x, newY);         
      }
   }

   if (!twistingHandControl)
   {
      // get speed from cursor position...
      POINT newMouse;
         
      // get the new mouse position...
      GetCursorPos(&newMouse);
      newMouse.x = 0;
      ::SetCursorPos(newMouse.x, newMouse.y);
      
      dy = centerY - newMouse.y;
   }

   if (AMainFrame::m_pref.reverseSpeed)
      dy = -dy;
   
   if (dy > 127) dy = 127;
   else if (dy < (short) -127) dy = (short) -127;
   mouse1.x = 0;  
   
   fSpeed = dy;
   
   // return values from speedTable....
   int index;
   if (dy < 0) index = -dy;
   else index = dy;                    
   deltaPixels = fDeltaPixelsTable[index];
   frameInterval = sIntervalTable[index];  
   return dy;
}


void CScrollDisplay::SetSpeed(short s)
{
   
   if (s > 127) s = 127;
   else if (s < (short) -127) s = (short) -127;
   if (fScrolling)
   {
      if (AMainFrame::m_pref.reverseSpeed)
         mouse1.y = centerY + s;
      else
         mouse1.y = centerY - s;
      ::SetCursorPos(mouse1.x, mouse1.y);
   }
   fSpeed = s;
}     
     

DWORD CScrollDisplay::GetCurrTime(void)
{
   return timeGetTime();   
}

void CScrollDisplay::DrawPiece(CDC *pCDC, CScrollPos *pPos, short nLine, short nHeight)
{
   co_ordinate savePos, off;
   paige_rec_ptr prp;
   pg_ref pg;
   RECT r;
#if MS_DEMO
   short demoPos, demoLine=nLine, demoHeight=nHeight;
#endif
   
   off.h = m_virtualDriverData->scroll_left;
   off.v = nLine;

   while (nHeight > 0)
   {
      // get current position, script
      long v = pPos->GetCurPos();

      // see how far to next script (also moves m_v, m_s)
      long run = pPos->ScriptRun(nHeight);
      if (run == 0) break;

      r.left = m_virtualDriverData->script_left;
      r.top = nLine; 
      r.right = m_virtualDriverData->script_right;
      r.bottom = nLine + run;

      CScriptView *script = pPos->GetCurScript();
      if ((AMainFrame::GetQueuePtr()->GetCount() > 0) && (script != NULL))
      {  rectangle lr;
         short chunkSize = r.left + r.right >> 1;
      
         pg = script->GetPgRef();
   
         // get old doc position, scroll to current
         prp = (paige_rec_ptr) UseMemory(pg);
         savePos = prp->scroll_pos;
         prp->scroll_pos.h = 0;
         prp->scroll_pos.v = v - script->m_start;
         UnuseMemory(pg);
   
         // erase drawing area
         pCDC->FillRect(&r, m_erase_brush);

         // set up our visible area to match drawing area
            lr.top_left.v = nLine;
            lr.bot_right.v = nLine + run;
            lr.top_left.h = r.left;
            lr.bot_right.h = r.right;
         pgSetShapeRect(m_vis, &lr);

         // draw the text piece
           pgDisplay(pg, &m_pg_dev, m_vis, NULL, &off, direct_or);


         // draw page seperator
              if (AMainFrame::m_pref.showDividers && (v - script->m_start <= 0))
              {   RECT inv;
                    
                 inv = r;
                 inv.bottom = inv.top + 2;
               pCDC->InvertRect(&inv);
              }
   
         prp = (paige_rec_ptr) UseMemory(pg);
         prp->scroll_pos = savePos;
         UnuseMemory(pg);

#define MIRROR_AMOUNT   16
         // mirror image if necessary
         if (m_opt & SDO_MIRROR)
         {  short y, yy, z, mw;
         
            y = nLine;
            mw = r.right - r.left; 
            z = nLine + run;
            while (y < z)
            {
               yy = z - y;
               if (yy > MIRROR_AMOUNT)
                  yy = MIRROR_AMOUNT;
               pCDC->StretchBlt(
                        r.left, y, 
                        mw, yy,
                        pCDC,
                        r.right-1, y, 
                        -mw, yy,
                        SRCCOPY);                         
               y += yy;
            }
         }
      } else {
         pCDC->FillRect(&r, m_erase_brush);
      }
      nLine += (short) run;
      nHeight -= (short) run;
      off.v += run;
   }

}

void CScrollDisplay::AddLineNum(short *nLine, short nDelta)
{
   *nLine += nDelta;
   if (*nLine > m_virtualDriverData->end_line) *nLine -= m_virtualDriverData->end_line;
   else if (*nLine < 0) *nLine += m_virtualDriverData->end_line;
}                                          


BOOL CScrollDisplay::ConfigDialog(void)
{
   return TRUE;
}


// CFullScreenWnd
IMPLEMENT_DYNCREATE(CFullScreenWnd, CWnd)
BEGIN_MESSAGE_MAP(CFullScreenWnd, CWnd)
   //{{AFX_MSG_MAP(CFullScreenWnd)
   ON_COMMAND(ID_FASTER_FRAMES, OnFasterFrames)
   ON_COMMAND(ID_SLOWER_FRAMES, OnSlowerFrames)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CFullScreenWnd::PreCreateWindow(CREATESTRUCT& cs)
{
   ASSERT(cs.lpszClass == NULL);       // must not be specified
   cs.lpszClass = AfxRegisterWndClass(cs.style,
      AfxGetApp()->LoadStandardCursor(IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1),
      AfxGetApp()->LoadIcon(rMainFrame));
   return TRUE;
}

BOOL CFullScreenWnd::Create(CWnd* pOwnerWnd)
{
   short w, h;
      
   ASSERT(pOwnerWnd != NULL);

   w = 500;//::GetSystemMetrics(SM_CXFULLSCREEN);
   h = 500;//::GetSystemMetrics(SM_CYFULLSCREEN) + 32; 
   fWidth = w;  
   fHeight = h;
   
//   return CreateEx(0, NULL, "", WS_POPUP | WS_VISIBLE, 0, 0, w, h, pOwnerWnd->GetSafeHwnd(), NULL, NULL);
   return CreateEx(0, NULL, "", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, w, h, pOwnerWnd->GetSafeHwnd(), NULL, NULL);
}

int CFullScreenWnd::GetHeight()
{
   return fHeight;
}            

int CFullScreenWnd::GetWidth()
{
   return fWidth;
}                 


DWORD gCount = 0;
DWORD prevTime = 0;


// NOTE: these globals will eventually have to be contained within a driver's own space
short gSaveMode = 0;

#if qWin16
   #include "toolhelp.h"
   TIMERINFO gTimerInfo;
#else
   #include "MMSystem.h"
#endif

#define WHACK_TIMER  0
//
//  VDMainScreen
//
DWORD VDMainScreen(MSVD_Data *m_virtualDriverData, WORD message)
{
  DWORD result = 0;
  CScrollDisplay *sd = &AMainFrame::m_display;
  
  switch (message)
  {
    case vdStartup:
   {
      m_virtualDriverData->driver_flags = VDF_TIMER | VDF_COLOR;  // capable of timer, color
      m_virtualDriverData->phys_width = 500;//GetSystemMetrics(SM_CXFULLSCREEN);
      m_virtualDriverData->phys_height = 500;//GetSystemMetrics(SM_CYFULLSCREEN) + GetSystemMetrics(SM_CYCAPTION);

      m_virtualDriverData->phys_planes = 1;  // default to B&W
      m_virtualDriverData->phys_depth = 1;
      if (m_virtualDriverData->sdo_flags & SDO_COLOR)
      {
         HDC dc = CreateDC("DISPLAY", NULL, NULL, NULL);
         if (dc != NULL)
         {
            m_virtualDriverData->phys_planes = GetDeviceCaps(dc, PLANES);
            m_virtualDriverData->phys_depth = GetDeviceCaps(dc, BITSPIXEL);
            DeleteDC(dc);
         }
      }
      m_virtualDriverData->scroll_left = 64;
      #if qWin16
         gTimerInfo.dwSize = sizeof(gTimerInfo);
      #endif
   }
   break;

   case vdEnterScroll:
      m_virtualDriverData->current_time = m_virtualDriverData->draw_time = 0;
      #if qWin16
         //$32 need to get time in millisecs
         TimerCount(&gTimerInfo);
         m_virtualDriverData->driverData[2] = gTimerInfo.dwmsSinceStart;
      #else
         m_virtualDriverData->driverData[2] = timeGetTime();
      #endif
      m_virtualDriverData->driverData[1] = 0;
   break;

   case vdUpdateScroll: // only called on initial render
   {  short line, lines, clip, x, y, w, h;
      HBITMAP xBitmap;

      x = m_virtualDriverData->copy_left;
      y = 0;
      
      w = m_virtualDriverData->copy_right - x;
      h = m_virtualDriverData->phys_height;

      xBitmap = (HBITMAP) ::SelectObject(m_virtualDriverData->script_dc, m_virtualDriverData->script_bm);
   
      line = m_virtualDriverData->top_line;
      m_virtualDriverData->draw_top = line;
      lines = h;
      clip = m_virtualDriverData->end_line - line;
      
         if (clip < lines)
         {
            BitBlt(m_virtualDriverData->screen_dc, x, y, // dest
               w,
               clip,
               m_virtualDriverData->script_dc,
               x, line,// source
               SRCCOPY);
            line = 0;
            lines -= clip;
         } else
            clip = 0;
   
         BitBlt(m_virtualDriverData->screen_dc, x, y + (clip),
            w,
            lines,
            m_virtualDriverData->script_dc,
            x, line, // source
            SRCCOPY);
   
         ::SelectObject(m_virtualDriverData->script_dc, xBitmap); //  
      m_virtualDriverData->copy_left = m_virtualDriverData->script_left;
      m_virtualDriverData->copy_right = m_virtualDriverData->script_right;
      m_virtualDriverData->dirty = FALSE;
   }
    break;
  }
  return result;
}


void CFullScreenWnd::OnFasterFrames()
{
   if(fMinFrameInterval > 6)  
   {
      fMinFrameInterval--;    
   }
   else
   {
      ::MessageBeep(-1);
   }
}

void CFullScreenWnd::OnSlowerFrames()
{     
   if(fMinFrameInterval < 70)
   {
      fMinFrameInterval++;
   }
   else
   {
      ::MessageBeep(-1);
   }
}

old code:
re-initialize (start over at position 0) scrolling each time a script is opened or closed

start scroll:
initialize, create wnd
vdEnterScroll, vdUpdateScroll
create scrollPosIndicator, draw timer (using control palette)
load scroll cursor and clip it to the left
draw piece (specifying whole screen)

on each timer tick:
check status of hand controller / mouse
if no controller, get speed from mouse position
if the frame rate display is up and it's timed-out, get rid of it and draw behind
once in a while check for WM_ messages: process KEYDOWN and SYSKEYDOWN
use CWnd::ScrollWindow!
clean up top and bottom borders
DrawSomeRows / DrawPiece

Exit:
GetCurrentScrollPos - set the word processor's scroll values, current location, script
turn on 'show invisibles' again, if needed
*/


void AScrollView::OnChangeSpeed() 
{
//   const bool decrease = (rCmdDecreaseSpeed == LOWORD(this->GetCurrentMessage()->wParam));
//   const char delta = (0x8000 & ::GetAsyncKeyState(VK_MENU)) ? 21 : 11;
//   this->SetSpeed(fSpeed + (decrease ? -delta : +delta));
}

void AScrollView::OnMouseMove(UINT nFlags, CPoint point) 
{
/*
   point.x = fMetrics.fWindow.left; // don't allow X movement
   const int deltaY = (fMetrics.fWindow.CenterPoint().y - 
    (fMetrics.fWindow.top + point.y));
   this->SetSpeed(deltaY);
*/

   CWnd::OnMouseMove(nFlags, point);
}
