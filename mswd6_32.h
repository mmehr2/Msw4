// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#ifndef hMswD6_32
#define hMswD6_32


#include "Msw.h"

typedef unsigned long (pascal *PFN_RTF_CALLBACK)(int, int);

extern "C" int pascal InitConverter32(HANDLE, char *);
extern "C" HANDLE pascal RegisterApp32(unsigned long, void *);
extern "C" int pascal IsFormatCorrect32(HANDLE, HANDLE);
extern "C" int pascal ForeignToRtf32(HANDLE, void *, HANDLE, HANDLE, HANDLE, PFN_RTF_CALLBACK);
extern "C" int pascal RtfToForeign32(HANDLE, LPSTORAGE, HANDLE, HANDLE, PFN_RTF_CALLBACK);


#endif // hMswD6_32