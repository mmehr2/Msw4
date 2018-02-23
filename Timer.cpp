// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

//-----------------------------------------------------------------------
// includes
#include "stdafx.h"

#include <iostream>
#include "Timer.h"

//-----------------------------------------------------------------------
// globals and externs

//-----------------------------------------------------------------------
// ATimer class


#ifdef _DEBUG
//-----------------------------------------------------------------------
// diagnostics
void ATimer::AssertValid() const
{
}

void ATimer::Dump() const
{
   ::_ftprintf_p(stderr, _T("ATimer: %d second(s)"), this->Elapsed() / 1000);
}
#endif // _DEBUG
