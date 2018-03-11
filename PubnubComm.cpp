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

static const std::string channel_separator = "-"; // avoid Channel name disallowed characters
static const std::string app_name = "MSW"; // prefix for all channel names by convention

//extern "C" {
   void pn_callback(pubnub_t* pn, pubnub_trans t, pubnub_res res, void* pData);
//}

class PNChannelInfo {
public:
   std::string key;
   std::string channelName;
   pubnub_t * pContext;
   //void * pConnection; //?? what waits for data??

   PNChannelInfo(/*pubnub_t * pC*/);
   ~PNChannelInfo();

   bool Init(bool is_primary);
   bool DeInit();

private:
   PNChannelInfo(const PNChannelInfo& other);
   PNChannelInfo& operator=(const PNChannelInfo& other);
};

PNChannelInfo::PNChannelInfo(/*pubnub_t * pC*/) 
   : key("demo")
   , pContext(nullptr)
   , channelName("")
   //, pConnection(nullptr)
{
}

bool PNChannelInfo::Init(bool isPrimary)
{
   pubnub_res res;
   if (pContext)
      pubnub_free(pContext);
   pContext = pubnub_alloc();
   if (isPrimary)
      pubnub_init(pContext, key.c_str(), ""); // publishes on this channel only
   else
      pubnub_init(pContext, "", key.c_str()); // subscribes on this channel only
   res = pubnub_register_callback(pContext, &pn_callback, (void*) this);
   if (res != PNR_OK)
      return false; // error reporting needed!
   return true;
}

bool PNChannelInfo::DeInit()
{
   if (pContext) {
      pubnub_free(pContext);
      pContext = nullptr;
   }
   return true;
}

//PNChannelInfo::PNChannelInfo(const PNChannelInfo& other) 
//   : key(other.key)
//   , pContext(nullptr)
//   , channelName(other.channelName)
//   //, pConnection(other.pConnection)
//{
//}

PNChannelInfo::~PNChannelInfo() 
{
   DeInit();
}


APubnubComm::APubnubComm(AComm* pComm) 
   : fParent(nullptr)
   , fComm(pComm)
   , fConnection(kNotSet)
   , fLinked(kDisconnected)
   , customerName("")
   , pLocal(nullptr)
   , pRemote(nullptr)
   //, pContext(nullptr)
{
   pLocal = new PNChannelInfo();
   pRemote = new PNChannelInfo();
   //std::string pubkey = "demo", subkey = "demo";
   // get these from persistent storage
   char buffer[1024];
   DWORD res;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "Pubkey", "demo", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: Pubkey returned %d: %s\n", res, buffer);
   this->pRemote->key = buffer;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "Subkey", "demo", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: Subkey returned %d: %s\n", res, buffer);
   this->pLocal->key = buffer;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "CompanyName", "", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: CompanyName returned %d: %s\n", res, buffer);
   this->customerName = buffer;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "DeviceName", "", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: DeviceName returned %d: %s\n", res, buffer);
   this->pLocal->channelName = this->MakeChannelName(buffer);
}


APubnubComm::~APubnubComm(void)
{
   // shut down links too?!!?! but cannot fail or wait
   delete pRemote;
   pRemote = nullptr;
   delete pLocal;
   pLocal = nullptr;
}

void APubnubComm::SetParent(HWND parent)
{
   // SECONDARY: will forward all incoming messages to this window, after receipt of incoming data from Service
   fParent = parent;
}

bool APubnubComm::ChangeMode(ConnectionType ct) {
   bool result = true;
   if (this->isSet()) {
      // any issues from switching between modes? check here
      this->fConnection = kNotSet;
   }
   if (ct != kNoChange && ct != kNotSet) {
      this->fConnection = ct;
   }
   return result;
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
   PNChannelInfo* pInfo = pLocal;
   if (this->isPrimary())
      pInfo = pRemote;
   result = pInfo->channelName.c_str();
   return result;
}

const char* APubnubComm::GetConnectionTypeName() const
{
   const char* result = this->isPrimary() ? "PRIMARY" : this->isSecondary() ? "SECONDARY" : "UNCONFIGURED";
   return result;
}

const char* APubnubComm::GetConnectionStateName() const
{
   const char* result = "";
   switch (fLinked) {
   case kDisconnected:
      result = "DISCONNECTED";
      break;
   case kConnected:
      result = "CONNECTED LOCALLY";
      break;
   case kChatting:
      result = "CONNECTED REMOTELY";
      break;
   case kScrolling:
      result = "SCROLLING";
      break;
   case kFileSending:
      result = "FILE SEND";
      break;
   case kFileRcving:
      result = "FILE RECEIVE";
      break;
   default:
      result = "UNKNOWN";
      break;
   }
   return result;
}


bool APubnubComm::Initialize(bool as_primary, const char* pLocalName_)
{
   this->Deinitialize();

   bool result = true;
   std::string pLocalName = pLocalName_;
   fConnection = as_primary ? kPrimary : kSecondary;
   if (!pLocalName.empty())
      pLocal->channelName = this->MakeChannelName(pLocalName);
   
   if (pLocal->channelName.empty()) {
      TRACE("%s IS UNCONFIGURED, UNABLE TO OPEN LOCAL CHANNEL LINK.\n");
      return false;
   }

   // TBD - make it so
   if (!ConnectHelper(pLocal)) {
      TRACE("%s IS UNABLE TO CONNECT TO %s ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), pLocalName_, pLocal->channelName.c_str());

      result = false;
   } else {
      TRACE("%s IS NOW LISTENING TO %s ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), pLocalName_, pLocal->channelName.c_str());

      fLinked = kConnected; // TBD - move this to callback when known
   }
   return result; // started sequence successfully
}

void APubnubComm::Deinitialize()
{
   if (fLinked == kDisconnected) {
      return;
   }

   if (pLocal->channelName.empty()) {
      TRACE("%s HAS NO LOCAL LINK TO SHUT DOWN.\n", this->GetConnectionTypeName());
      return;
   }

   // shut down any pRemote link first
   this->CloseLink();

   TRACE("%s IS SHUTTING DOWN LOCAL LINK TO PUBNUB CHANNEL %s\n", this->GetConnectionTypeName(), pLocal->channelName.c_str());
   // make it so 
   // just don't lose the pLocal channel name - it will be replaced if needed by a new one, else use old one
   if (!DisconnectHelper(pLocal)) {
      TRACE("%s IS UNABLE TO SHUT DOWN CHANNEL %s\n", 
         this->GetConnectionTypeName(), pLocal->channelName.c_str());
      // TBD - but this really will cause problems! what are failure scenarios here?
   } else {
      TRACE("%s CORRECTLY SHUT DOWN CHANNEL %s\n", 
         this->GetConnectionTypeName(), pLocal->channelName.c_str());

      fLinked = kDisconnected;
   }
}

bool APubnubComm::OpenLink(const char * pRemoteName_)
{
   this->CloseLink();

   bool result = true;
   this->pRemote->channelName = this->MakeChannelName(pRemoteName_);
   TRACE("%s IS OPENING LINK TO SECONDARY %s ON PUBNUB CHANNEL %s\n", 
      this->GetConnectionTypeName(), pRemoteName_, this->pRemote->channelName.c_str());

   // TBD - move this to callback when known
   if (!ConnectHelper(pRemote)) {
      TRACE("%s IS UNABLE TO CONNECT TO %s ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), pRemoteName_, pRemote->channelName.c_str());

      result = false;
   } else {
      TRACE("%s IS NOW READY TO SEND TO %s ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), pRemoteName_, pRemote->channelName.c_str());

      fLinked = kChatting;
   }

   return result; // started sequence successfully
}

void APubnubComm::CloseLink()
{
   if (fLinked < kChatting) {
      return; // already closed
   }

   if (pRemote->channelName.empty()) {
      TRACE("%s HAS NO REMOTE LINK TO SHUT DOWN.\n", this->GetConnectionTypeName());
      return;
   }

   // shut down any higher states here

   TRACE("%s IS CLOSING LINK TO SECONDARY ON PUBNUB CHANNEL %s\n", 
      this->GetConnectionTypeName(), this->pRemote->channelName.c_str());

   // TBD: make it so
    if (!DisconnectHelper(pRemote)) {
      TRACE("%s IS UNABLE TO SHUT DOWN CHANNEL %s\n", 
         this->GetConnectionTypeName(), pRemote->channelName.c_str());
      // TBD - but this really will cause problems! what are failure scenarios here?
   } else {
      TRACE("%s CORRECTLY SHUT DOWN CHANNEL %s\n", 
         this->GetConnectionTypeName(), pRemote->channelName.c_str());

      fLinked = kConnected;
   }
 }

void APubnubComm::SendCommand(const char * message)
{
   //bool result = true;
   TRACE("SENDING MESSAGE %s\n", message); // DEBUGGING
   // TBD: publish message to pRemote->channelName and set up callback
   // PRIMARY: will send commands
   // SECONDARY: will send any responses
}

void APubnubComm::OnMessage(const char * message)
{
   //bool result = true;
   TRACE("RECEIVED MESSAGE %s\n", message); // DEBUGGING
   // this is the callback for messages received on pLocal->channelName
   // PRIMARY: will receive any responses
   // SECONDARY: will receive commands
}

// PUBNUB IMPLEMENTATION

void pn_callback(pubnub_t* pn, pubnub_trans t, pubnub_res res, void* pData)
{
   //Print the current thread ID in the Debug Window
   TRACE(_T("Current Thread ID = 0x%X\n"), ::AfxGetThread()->m_nThreadID);
}

bool APubnubComm::ConnectHelper(PNChannelInfo* pChannel)
{
   bool result = false;

   TRACE(_T("CONNHELP: Current Thread ID = 0x%X\n"), ::AfxGetThread()->m_nThreadID);

   if (pChannel == nullptr)
   {
      // need a valid one here - setup with proper key and channel name
      TRACE("CH> DAMN!\n");
      return false;
   }

   if (pChannel->Init(this->isPrimary()))
   {
      TRACE("CH> NEW %s CHANNEL %X\n", this->GetConnectionTypeName(), (LPVOID)pChannel->pContext);
      result = true;
   }

   return result;
}

bool APubnubComm::DisconnectHelper(PNChannelInfo* pChannel)
{
   bool result = false;

   LPVOID pCtx = (LPVOID)pChannel->pContext; // before it goes away!
   TRACE(_T("DISCONNHELP: Current Thread ID = 0x%X, CTX=%X\n"), ::AfxGetThread()->m_nThreadID, pCtx);

   if (pChannel->DeInit())
   {
      TRACE("CH> DELETED %s CHANNEL %X\n", this->GetConnectionTypeName(), pCtx);
      result = true;
   }

   return result;
}
