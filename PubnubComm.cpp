#include "stdafx.h"
#include "PubnubComm.h"

#include "Comm.h"

//#define PUBNUB_THREADSAFETY
#define PUBNUB_USE_EXTERN_C (1)
#include "pubnub.hpp"
#include <iostream>

#pragma comment (lib, "pubnub_callback")
//#pragma comment (lib, "pubnub_sync")


APubnubComm::APubnubComm(AComm* pComm) 
   : fParent(NULL)
   , fComm(pComm)
{
}


APubnubComm::~APubnubComm(void)
{
}

void APubnubComm::SetParent(HWND parent)
{
   // SECONDARY: will forward all incoming messages to this window, after receipt of incoming data from Service
   fParent = parent;
}

bool APubnubComm::Connect(bool as_primary)
{
   fPrimary = as_primary;
   // PRIMARY: only needs to publish during scrolling
   // SECONDARY: only needs to subscribe during scrolling
   // to send the script file? we'll look into it... for now just use existing text (agreed upon in advance)
   TRACE("SET UP LINK TO PUBNUB... AS %s\n", fPrimary ? "PRIMARY" : "SECONDARY");

   try {
        enum pubnub_res res = PNR_OK;
        char const *chan = "hello_world";
        //pubnub::context pb("demo", "demo");
      TRACE("Demo PUBSUB is ON! Results: %d\n", res);
   } catch (std::exception x) {
      TRACE("DEMO PUBSUB ERROR: %s", x.what());
   }

   return true; // started sequence successfully
}
