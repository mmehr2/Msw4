// Copyright © 2004 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: ScrollView.h,v 1.6 2004/01/15 18:16:11 scox Exp $


#ifndef hScrollView
#define hScrollView


//#include "MSW.h"

//#include "MagicScroll.h"


//-----------------------------------------------------------------------

class AScrollView : public CWnd
{
public:
   AScrollView();

   bool Initialize(CWnd* editView);
   bool Uninitialize();

   //{{AFX_VIRTUAL(AScrollView)
	//}}AFX_VIRTUAL

private:
   //{{AFX_MSG(AScrollView)
	afx_msg void OnPaint();
	afx_msg void OnClose();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnDestroy();
	afx_msg void OnPause();
	afx_msg void OnToggleScrollMode();
	afx_msg void OnChangeSpeed();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private: // classes
   class AMetrics
   {
   public:
      AMetrics();
      void Update();

      CSize fScreenSize;
      CRect fWindow;
      CRect fClient;
   } fMetrics;

private: // data
   /// DirectDraw
   HRESULT ProcessNextFrame();
   HRESULT DisplayFrame();
   HRESULT RestoreSurfaces();

   bool Scroll();
   void SetSpeed(int speed);
   void SetPaperclip();
   void GotoPaperclip();

   HACCEL fAccellerators;
//   CDisplay fDisplay;
//   CSurface* fSurface;
   bool fFullScreen;
   bool fScrolling;
   int fSpeed, fSavedSpeed;
   CPoint fScrollPos;
   CWnd* fEditView;
}; // AScrollView


//-----------------------------------------------------------------------
// inline functions
inline AScrollView::AMetrics::AMetrics()
{
   this->Update();
}

//{{AFX_INSERT_LOCATION}}


extern AScrollView gScrollView;


/*
const SInt16 kMaxVelocity = 600;

// Structs
struct RowImage
{
   SInt32 position;
   SInt32 height;
   bool drawn;
   HBITMAP image;
};

typedef std::vector<RowImage> RowImageVector;


struct RowSurface
{
   SInt32 imageNumber;
   SInt32 currentPosition;
   CSurface* imageSurface;
};

typedef std::deque<RowSurface> RowSurfaceDeque;
typedef RowSurfaceDeque::iterator RsdIterator;
typedef RowSurfaceDeque::reverse_iterator RsdReverseIterator;

struct SurfaceRange
{
   SInt32 startRow;
   SInt32 endRow;
   SInt32 startOffset;
};

typedef std::vector<SurfaceRange> SurfaceRangeVector;


class AScrollView
{
public:
   AScrollView();
   virtual ~AScrollView();

   bool EnterScrollMode();
   bool ExitScrollMode();
   
   bool SwitchScrollMode(AMagicScrollView* view);
   HRESULT WinInit(HINSTANCE hInst, SInt16 nCmdShow, HWND* phWnd, HACCEL* phAccel);
   HRESULT InitDirectDraw(HWND hWnd);
   void FreeDirectDraw();
   HRESULT ProcessNextFrame();
   void MoveSurfaces();
   HRESULT DisplayFrame();
   HRESULT RestoreSurfaces();

public:
   AMagicScrollView* fMagicScrollView;
   SInt32 fSelectionStart;
   SInt32 fSelectionEnd;
   HBITMAP fDocumentImage;
   HBITMAP fScreenBackground;
   SInt32 fImageHeight;
   CDisplay* fScrollDisplay;
   CSurface* fTextSurfaceFirst;
   CSurface* fTextSurfaceSecond;
   CSurface* fTextSurfaceThird;
   bool fScrollMode;
   bool fWindowVisible; 

   UInt32 fTimer;
   UInt32 fLastTick;
   HWND fScrollWindow;
   SInt32 fScrollVelocity;
   SInt32 fCurrentPosition;
   SInt32 fCurrentDocumentPosition;
   SInt32 fCurrentSurface;

   SInt32 fFirstTopRow;
   SInt32 fSecondTopRow;
   SInt32 fFirstTopOffset;
   SInt32 fSecondTopOffset;

   RowImageVector fRowImage;
   SInt32 fRowCount;
   RowSurfaceDeque fRowSurface;
   SurfaceRangeVector fSurfaceRange;
   SInt32 fSurfaceCount;
   SInt32 fFirstSurfaceRange;
   SInt32 fSecondSurfaceRange;
   Led_Bitmap fFirstSurfaceBitmap;
   Led_Bitmap fSecondSurfaceBitmap;
};

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
VOID CALLBACK ScrollTimerProc(UINT wTimerID, UINT wMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
extern AScrollView* gScrollViewObject;
*/

//{{AFX_INSERT_LOCATION}}


#endif // hScrollView
