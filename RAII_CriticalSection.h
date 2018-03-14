#pragma once

#include <afx.h>

// instantiate this to enter a critical section with auto-leave on destruction (exception, leave function, etc.)
class RAII_CriticalSection
{
   LPCRITICAL_SECTION pcs;
public:
   RAII_CriticalSection(LPCRITICAL_SECTION pCS)
      : pcs(pCS)
   {
      ::EnterCriticalSection(pcs);
   }
   ~RAII_CriticalSection()
   {
      ::LeaveCriticalSection(pcs);
   }
};

