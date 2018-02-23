#include "MemHogDlg.h"
#include <vector>

struct Alloc {char* buffer; size_t size;};
typedef std::vector<Alloc> Allocs;
static Allocs gAllocs;

CMemHogDlg::CMemHogDlg(CWnd* pParent /*=NULL*/)	:
   CDialog(CMemHogDlg::IDD, pParent),
   fAllocate(0)
   , fStatus(_T(""))
   {
}

void CMemHogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   ::DDX_Text(pDX, IDC_ALLOCATE, fAllocate);
   ::DDX_Text(pDX, IDC_STATUS, fStatus);
}

BEGIN_MESSAGE_MAP(CMemHogDlg, CDialog)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDOK, &CMemHogDlg::OnAllocate)
   ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CMemHogDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

   this->Update();
   this->SetTimer(0, 1000, NULL);

   return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMemHogDlg::Update()
{
   MEMORYSTATUS status;
   ::GlobalMemoryStatus(&status);
   this->SetDlgItemInt(IDC_TOTAL_PHYSICAL, status.dwTotalPhys / (1024 * 1024), FALSE);
   this->SetDlgItemInt(IDC_REMAINING, status.dwAvailPhys / (1024 * 1024), FALSE);
   this->SetDlgItemInt(IDC_VREMAINING, status.dwAvailVirtual / (1024 * 1024), FALSE);
}

void CMemHogDlg::OnAllocate()
{
   this->UpdateData();

   fStatus.Empty();

   Alloc alloc;
   alloc.size = fAllocate * 1024 * 1024;
   alloc.buffer = (char*)::GlobalAlloc(GMEM_FIXED, alloc.size);
   if (NULL != alloc.buffer)
      gAllocs.push_back(alloc);
   else
      fStatus = _T("*** Failed ***");

   this->UpdateData(FALSE);
}

void CMemHogDlg::OnTimer(UINT_PTR nIDEvent)
{
   this->Update();

   // now access some of the memory so that pages are kept in physical RAM
   for (Allocs::const_iterator i = gAllocs.begin(); i != gAllocs.end(); ++i)
   {
      if (i->size > 0)
      {
         i->buffer[0] = 1;
         i->buffer[i->size - 1] = 2;
      }
   }

   CDialog::OnTimer(nIDEvent);
}
