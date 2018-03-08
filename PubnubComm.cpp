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
   , fConnection(kNotSet)
   , fLinked(kDisconnected)
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

bool APubnubComm::Initialize(ConnectionType conn_type)
{
   bool result = true;
   //fPrimary = as_primary;
   // PRIMARY: only needs to publish during scrolling
   // SECONDARY: only needs to subscribe during scrolling
   // to send the script file? we'll look into it... for now just use existing text (agreed upon in advance)
   TRACE("SET UP LINK TO PUBNUB... AS %s\n", this->isPrimary() ? "PRIMARY" : "SECONDARY");

   try {
        enum pubnub_res res = PNR_OK;
        char const *chan = "hello_world";
        pubnub::context pb("demo", "demo");
      TRACE("Demo PUBSUB is ON! Results: %d\n", res);
   } catch (std::exception x) {
      TRACE("DEMO PUBSUB ERROR: %s", x.what());
   }

   return result; // started sequence successfully
}

void APubnubComm::Deinitialize()
{
   TRACE("SHUTTING DOWN LINK TO PUBNUB... AS %s\n", this->isPrimary() ? "PRIMARY" : "SECONDARY");
}

bool APubnubComm::OpenLink(const char * address)
{
   bool result = true;
   TRACE("OPEN LINK TO SECONDARY OVER PUBNUB... AS PRIMARY\n");
   return result; // started sequence successfully
}

bool APubnubComm::CloseLink()
{
   bool result = true;
   TRACE("CLOSE LINK TO SECONDARY OVER PUBNUB... AS PRIMARY\n");
   return result; // started sequence successfully
}
