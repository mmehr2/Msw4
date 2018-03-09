#include "stdafx.h"
#include "PubnubComm.h"

#include "Comm.h"

//#define PUBNUB_THREADSAFETY
#define PUBNUB_USE_EXTERN_C 1
#include "pubnub.hpp"
#include <iostream>
#include <functional>

#pragma comment (lib, "pubnub_callback")
//#pragma comment (lib, "pubnub_sync")


APubnubComm::APubnubComm(AComm* pComm) 
   : fParent(NULL)
   , fComm(pComm)
   , fConnection(kNotSet)
   , fLinked(kDisconnected)
   , pubkey("")
   , subkey("")
   , pPCPublic(NULL)
   , publicChannelName("")
   , pPCChat(NULL)
   , chatChannelName("")
//   , pConnecter(NULL)
//   , pConversation(NULL)
{
   std::string pubkey = "demo", subkey = "demo";
   // get these from persistent storage
   char buffer[1024];
   DWORD res = ::GetPrivateProfileStringA("MSW_Pubnub", "Pubkey", "demo", buffer, sizeof(buffer), "msw.ini");
   TRACE("PUBKEY returned %d: %s\n", res, buffer);
   this->pubkey = buffer;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "Subkey", "demo", buffer, sizeof(buffer), "msw.ini");
   TRACE("SUBKEY returned %d: %s\n", res, buffer);
   this->subkey = buffer;
}


APubnubComm::~APubnubComm(void)
{
}

void APubnubComm::SetParent(HWND parent)
{
   // SECONDARY: will forward all incoming messages to this window, after receipt of incoming data from Service
   fParent = parent;
}

bool APubnubComm::Initialize(bool as_primary, const char* publicName)
{
   this->Deinitialize();

   bool result = true;
   fConnection = as_primary ? kPrimary : kSecondary;
   // PRIMARY: only needs to publish during scrolling
   // SECONDARY: only needs to subscribe during scrolling
   // to send the script file? we'll look into it... for now just use existing text (agreed upon in advance)
   TRACE("SET UP LINK TO PUBNUB... AS %s\n", this->isPrimary() ? "PRIMARY" : "SECONDARY");

   pubnub::context pb(pubkey, subkey);
   TRACE("TEST PUBSUB is listening to channel %s!\n", publicName);
   publicChannelName = publicName;

   return result; // started sequence successfully
}

void APubnubComm::Deinitialize()
{
   if (fLinked == kDisconnected)
      return;

   TRACE("SHUTTING DOWN LINK TO PUBNUB... AS %s\n", this->isPrimary() ? "PRIMARY" : "SECONDARY");
   // TBD - call unsubscribe here?
   // remove conversation if in use
   if (!chatChannelName.empty())
      TRACE("REMOVING PRIVATE CONVERSATION %s\n", chatChannelName.c_str());
//   delete pConversation;
//   pConversation = NULL;
   chatChannelName = "";
   // remove context here ??
   fLinked = kDisconnected;
}

bool APubnubComm::OpenLink(const char * address)
{
   bool result = true;
   TRACE("OPENING LISTENER TO SECONDARY OVER PUBNUB... AS PRIMARY\n");
   return result; // started sequence successfully
}

bool APubnubComm::CloseLink()
{
   bool result = true;
   TRACE("CLOSE LINK TO SECONDARY OVER PUBNUB... AS PRIMARY\n");
   return result; // started sequence successfully
}

void APubnubComm::SendCommand(const char * message)
{
   bool result = true;
   TRACE("SENDING MESSAGE %s\n", message);
}

void APubnubComm::OnMessage(const char * message)
{
   bool result = true;
   TRACE("RECEIVED MESSAGE %s\n", message);
}

