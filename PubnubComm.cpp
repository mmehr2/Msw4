#include "stdafx.h"
#include "PubnubComm.h"

#include "Comm.h"

#define PUBNUB_CALLBACK_API
extern "C" {
#include "pubnub_callback.h"
}
//#include <iostream>
#include <string>

#pragma comment (lib, "pubnub_callback")
//#pragma comment (lib, "pubnub_sync")

static const std::string channel_separator = "-"; // avoid Channel name disallowed characters
static const std::string app_name = "Msw"; // prefix for all channel names by convention

PNChannelInfo::PNChannelInfo() 
   : key("demo")
   , pContext(nullptr)
   , channelName("")
   , pConnection(nullptr)
{
}

APubnubComm::APubnubComm(AComm* pComm) 
   : fParent(nullptr)
   , fComm(pComm)
   , fConnection(kNotSet)
   , fLinked(kDisconnected)
   , customerName("")
{
   //std::string pubkey = "demo", subkey = "demo";
   // get these from persistent storage
   char buffer[1024];
   DWORD res;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "Pubkey", "demo", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: Pubkey returned %d: %s\n", res, buffer);
   this->remote.key = buffer;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "Subkey", "demo", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: Subkey returned %d: %s\n", res, buffer);
   this->local.key = buffer;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "CompanyName", "", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: CompanyName returned %d: %s\n", res, buffer);
   this->customerName = buffer;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "DeviceName", "", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: DeviceName returned %d: %s\n", res, buffer);
   this->local.channelName = this->MakeChannelName(buffer);
}


APubnubComm::~APubnubComm(void)
{
}

void APubnubComm::SetParent(HWND parent)
{
   // SECONDARY: will forward all incoming messages to this window, after receipt of incoming data from Service
   fParent = parent;
}

std::string APubnubComm::MakeChannelName( const std::string& deviceName )
{
   std::string result;
   result += app_name;
   result += channel_separator;
   result += this->customerName;
   result += channel_separator;
   result += deviceName;
   return result;
}

bool APubnubComm::Initialize(bool as_primary, const char* localName)
{
   this->Deinitialize();

   bool result = true;
   fConnection = as_primary ? kPrimary : kSecondary;
   local.channelName = this->MakeChannelName(localName);

   TRACE("%s IS NOW LISTENING TO %s ON CHANNEL %s\n", 
      this->isPrimary() ? "PRIMARY" : "SECONDARY", localName, local.channelName.c_str());

   return result; // started sequence successfully
}

void APubnubComm::Deinitialize()
{
   if (fLinked == kDisconnected)
      return;

   TRACE("%s IS SHUTTING DOWN LOCAL LINK TO PUBNUB CHANNEL %s", this->isPrimary() ? "PRIMARY" : "SECONDARY", local.channelName.c_str());
   // TBD - call unsubscribe here?
   // remove conversation if in use
   if (!remote.channelName.empty())
      TRACE(",\n AND SHUTTING DOWN REMOTE LINK TO CHANNEL %s", remote.channelName.c_str());
//   delete remote.pConnection;
//   remote.pConnection = nullptr;
   remote.channelName = "";
   TRACE("\n");
   // remove context here ??
   fLinked = kDisconnected;
}

bool APubnubComm::OpenLink(const char * address)
{
   bool result = true;
   this->remote.channelName = this->MakeChannelName(address);
   TRACE("%s IS OPENING LINK TO SECONDARY %s ON PUBNUB CHANNEL %s\n"
      , this->isPrimary() ? "PRIMARY" : "SECONDARY", address, this->remote.channelName.c_str());
   // TBD: make it so
   return result; // started sequence successfully
}

bool APubnubComm::CloseLink()
{
   bool result = true;
   TRACE("%s IS CLOSING LINK TO SECONDARY ON PUBNUB CHANNEL %s\n"
      , this->isPrimary() ? "PRIMARY" : "SECONDARY", this->remote.channelName.c_str());
   // TBD: make it so
   return result; // started sequence successfully
}

void APubnubComm::SendCommand(const char * message)
{
   //bool result = true;
   TRACE("SENDING MESSAGE %s\n", message); // DEBUGGING
   // TBD: publish message to remote.channelName and set up callback
   // PRIMARY: will send commands
   // SECONDARY: will send any responses
}

void APubnubComm::OnMessage(const char * message)
{
   //bool result = true;
   TRACE("RECEIVED MESSAGE %s\n", message); // DEBUGGING
   // this is the callback for messages received on local.channelName
   // PRIMARY: will receive any responses
   // SECONDARY: will receive commands
}

