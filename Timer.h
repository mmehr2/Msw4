// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

//-----------------------------------------------------------------------
// includes
#include <ctime>

#pragma warning (push)
#pragma warning (disable: 4201)
#	include "mmsystem.h"

//-----------------------------------------------------------------------
// classes

class ATimer
{
public:
   // create a timer, initially stopped
   ATimer();
   virtual ~ATimer();

   // create a timer, initially stopped, but with 'milliseconds' elapsed 
   // time
   ATimer(DWORD milliseconds);

   DWORD Now() const;
   void Start();
   void Stop();
   void Resume();

   // reset the timer to 0
   void Reset();

   // returns milliseconds
   DWORD Elapsed() const;
   bool IsRunning() const;

#ifdef _DEBUG
   virtual void AssertValid() const;
   virtual void Dump() const;
#endif // qDebug

private: // data
   DWORD fStart, fEnd;
   bool fRunning;
}; // ATimer


//-----------------------------------------------------------------------
// inline functions
inline ATimer::ATimer() :
   fRunning(false),
   fStart(0),
   fEnd(0)
{
   static bool initialized = false;
   if (!initialized)
   {  // this call sets the granularity for the timer. This is crucial
      // for optimal scrolling smoothness.
      ::timeBeginPeriod(1);
      initialized = true;
   }

   mDebugOnly(this->AssertValid());
}

inline ATimer::~ATimer()
{
   mDebugOnly(this->AssertValid());
}

inline ATimer::ATimer(DWORD milliseconds) :
   fRunning(false)
{
   fEnd = this->Now();
   fStart = fEnd - milliseconds;
}

inline void ATimer::Start()
{
   if (!this->IsRunning())
   {
      fStart = this->Now();
      fEnd = 0;
      fRunning = true;
   }
   mDebugOnly(this->AssertValid());
}

inline void ATimer::Stop()
{
   if (this->IsRunning())
   {
      fEnd = this->Now();
      fRunning = false;
   }
   mDebugOnly(this->AssertValid());
}

inline void ATimer::Resume()
{
   if (!this->IsRunning())
   {
      const DWORD elapsed = (fEnd - fStart);
      this->Start();
      fStart -= elapsed;
   }
   mDebugOnly(this->AssertValid());
}

inline void ATimer::Reset()
{
   fStart = fEnd = this->Now();
   mDebugOnly(this->AssertValid());
}

inline DWORD ATimer::Elapsed() const
{
   DWORD milliseconds = 0;
   if (!this->IsRunning())
      milliseconds = (fEnd - fStart);
   else
      milliseconds = (this->Now() - fStart);

   mDebugOnly(this->AssertValid());
   return milliseconds;
}

inline bool ATimer::IsRunning() const
{
   return fRunning;
}

inline DWORD ATimer::Now() const
{
   return ::timeGetTime();
}

#pragma warning (pop)
