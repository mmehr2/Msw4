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

const char* APubnubComm::GetConnectionName() const
{
   const char* result = "";
   return result;
}

const char* APubnubComm::GetConnectionTypeName() const
{
   const char* result = this->isPrimary() ? "PRIMARY" : "SECONDARY";
   return result;
}

const char* APubnubComm::GetConnectionStateName() const
{
   const char* result = "";
   return result;
}


bool APubnubComm::Initialize(bool as_primary, const char* localName_)
{
   this->Deinitialize();

   bool result = true;
   std::string localName = localName_;
   fConnection = as_primary ? kPrimary : kSecondary;
   if (!localName.empty())
      local.channelName = this->MakeChannelName(localName);
   
   if (local.channelName.empty()) {
      TRACE("%s IS UNCONFIGURED, UNABLE TO OPEN LOCAL CHANNEL LINK.\n");
      return false;
   }

   TRACE("%s IS NOW LISTENING TO %s ON CHANNEL %s\n", 
      this->GetConnectionTypeName(), localName_, local.channelName.c_str());

   // TBD - make it so

   fLinked = kConnected; // TBD - move this to callback when known
   return result; // started sequence successfully
}

bool APubnubComm::Deinitialize()
{
   if (fLinked == kDisconnected)
      return true;

   bool result = true;

   if (local.channelName.empty()) {
      TRACE("%s HAS NO LOCAL LINK TO SHUT DOWN.\n", this->GetConnectionTypeName());
      return false;
   }

   // shut down any remote link first
   this->CloseLink();

   TRACE("%s IS SHUTTING DOWN LOCAL LINK TO PUBNUB CHANNEL %s\n", this->GetConnectionTypeName(), local.channelName.c_str());
   // TBD - make it so 
   // just don't lose the local channel name - it will be replaced if needed by a new one, else use old one
   fLinked = kDisconnected;
   return result;
}

bool APubnubComm::OpenLink(const char * address)
{
   this->CloseLink();

   bool result = true;
   this->remote.channelName = this->MakeChannelName(address);
   TRACE("%s IS OPENING LINK TO SECONDARY %s ON PUBNUB CHANNEL %s\n", 
      this->GetConnectionTypeName(), address, this->remote.channelName.c_str());

   // TBD: make it so

   return result; // started sequence successfully
}

bool APubnubComm::CloseLink()
{
   bool result = true;

   if (fLinked == kDisconnected) {
      return result;
   }
   TRACE("%s IS CLOSING LINK TO SECONDARY ON PUBNUB CHANNEL %s\n", 
      this->GetConnectionTypeName(), this->remote.channelName.c_str());
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

