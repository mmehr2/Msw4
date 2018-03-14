#include "stdafx.h"
#include "PubnubComm.h"
#include "ChannelInfo.h"
#include "Comm.h"

#define PUBNUB_CALLBACK_API
extern "C" {
   void pn_printf(char* fmt, ...);

#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
}
//#include <iostream>
#include <string>

#pragma comment (lib, "pubnub_callback")

// channel naming convention
static const std::string channel_separator = "-"; // avoid Channel name disallowed characters
static const std::string app_name = "MSW"; // prefix for all channel names by convention

//// function to capture pubnub_log output to the TRACE() window (ONLY FOR OUR CODE)
//void pn_printf(char* fmt, ...)
//{
//   char buffer[10240];
//
//    va_list args;
//    va_start(args,fmt);
//    vsprintf(buffer,fmt,args);
//    va_end(args);
//
//    TRACE("%s\n", buffer);
//}

static LPCTSTR GetFullPathOfExeFile()
{
   const int BSIZE = 8192;
   // NOTE: For a discussion of Unicode path name length, which could theoretically exceed 32K WCHARs,
   //   see https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
   // The macro MAX_PATH is only 260 and is inadequate on today's drives (Microsoft, pay attention!)
   static TCHAR buffer[BSIZE];
   buffer[0] = '\0'; // in case call fails
   /*DWORD res =*/ ::GetModuleFileName( NULL, buffer, BSIZE);
   // res == 0 if failure, otherwise # of TCHARs copied
   return buffer;
}

static LPCTSTR GetIniFilePath()
{
   // NOTE: Full Unicode support required here
   LPCTSTR fpath = GetFullPathOfExeFile();
   LPCTSTR bslash = wcsrchr(fpath, '\\');
   bslash++; // include the backslash
   // end the string there (still sitting in original static buffer)
   LPTSTR nullspot = const_cast<LPTSTR>(bslash);
   nullspot[0] = '\0';
   return fpath;
}

LPCTSTR GetRemoteIniValue( LPCTSTR keyName, LPCTSTR defValue = _T("") )
{
   // NOTE: The params are TCHAR-style because file paths can include Unicode
   std::wstring iniFilePath = GetIniFilePath();
   iniFilePath += _T("msw.ini");
   LPCTSTR ini_path = iniFilePath.c_str();
   static TCHAR buffer[1024];
   DWORD res;
   LPCTSTR secName = _T("MSW_Pubnub");
   buffer[0] = '\0';
   res = ::GetPrivateProfileString(secName, keyName, defValue, buffer, sizeof(buffer), ini_path);
   TRACE(_T("CONFIG: %s returned %d: %s\n"), keyName, res, buffer);
   return buffer;
}

const char* GetRemoteIniValueA( LPCTSTR keyName, LPCTSTR defValue = _T("") )
{
   LPCTSTR buffer = GetRemoteIniValue(keyName, defValue);
   CT2A ascii(buffer);
   static std::string s;
   s = ascii.m_psz; // remember lifetime issues, but use assignment
   return s.c_str();
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
   //::InitializeCriticalSectionAndSpinCount(&cs, 0x400);
   //// spin count is how many loops to spin before actually waiting (on a multiprocesssor system)

   pLocal = new PNChannelInfo(this);
   pRemote = new PNChannelInfo(this);

   //std::string pubkey = "demo", subkey = "demo";
   // get these from persistent storage
   // NOTE: by using PrivateProfile, I can guarantee a file will be found (in the Windows directory, since I don't specify the full path)
   // Otherwise, Windows prefers that you use the registry, which is much harder for the customer to edit when configuring their own accounts.
   LPCTSTR keyname_pub = _T("Pubkey");
   LPCTSTR keyname_sub = _T("Subkey");
   LPCTSTR default_pnkey = _T("demo");
   LPCTSTR keyname_company = _T("CompanyName");
   LPCTSTR keyname_device = _T("DeviceName");
   this->pRemote->key = GetRemoteIniValueA(keyname_pub, default_pnkey);
   this->pLocal->key = GetRemoteIniValueA(keyname_sub, default_pnkey);
   // fix for publish "Invalid subscribe key" error
   this->pRemote->key2 = this->pLocal->key;
   // load the company/customer and device IDs (could be Unicode here - what does CT2A do with it?)
   this->customerName = GetRemoteIniValueA(keyname_company);
   const char* device_name = GetRemoteIniValueA(keyname_device);
   this->pLocal->channelName = this->MakeChannelName(device_name);
}


APubnubComm::~APubnubComm(void)
{
   // shut down links too?!!?! but cannot fail or wait
   delete pRemote;
   pRemote = nullptr;
   delete pLocal;
   pLocal = nullptr;
   //::DeleteCriticalSection(&cs);
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

bool APubnubComm::ConnectHelper(PNChannelInfo* pChannel)
{
   bool result = false;

   TRACE(_T("CONNHELP: Current Thread ID = 0x%X\n"), ::GetCurrentThreadId());

   // need a valid one here - setup with proper key and channel name
   ASSERT(pChannel != nullptr);

   bool is_send = pChannel == this->pRemote;
   if (pChannel->Init(is_send))
   {
      TRACE("CH> NEW %s CHANNEL %X\n", pChannel->GetTypeName(), (LPVOID)pChannel->pContext);
      result = true;
   }

   return result;
}

bool APubnubComm::DisconnectHelper(PNChannelInfo* pChannel)
{
   bool result = false;

   ASSERT(pChannel != nullptr);

   LPVOID pCtx = (LPVOID)pChannel->pContext; // before it goes away!
   TRACE(_T("DISCONNHELP: Current Thread ID = 0x%X, CTX=%X\n"), ::GetCurrentThreadId(), pCtx);

   if (pChannel->DeInit())
   {
      TRACE("CH> DELETED %s CHANNEL %X\n", pChannel->GetTypeName(), pCtx);
      result = true;
   }

   return result;
}

void APubnubComm::SendCommand(const char * message)
{
   //bool result = true;

   // TBD: publish message to pRemote->channelName and set up callback
   // PRIMARY: will send commands
   // SECONDARY: will send any responses
   std::string msg = (message);
   TRACE("SENDING MESSAGE %s:", msg.c_str()); // DEBUGGING
   if (false == pRemote->Send(message)) {
      TRACE(pRemote->op_msg.c_str());
   }
   TRACE("\n");
}

#include "Msw.h"
#include "RtfHelper.h"
#include "ScrollDialog.h"

void APubnubComm::OnMessage(const char * message)
{
   //bool result = true;
   TRACE("RECEIVED MESSAGE ON THREAD 0x%X:%s\n", ::GetCurrentThreadId(), message); // DEBUGGING
   // this is the callback for messages received on pLocal->channelName
   // PRIMARY: will receive any responses
   // SECONDARY: will receive commands
   std::string s = message;
   const char command = s[0];
   const long arg1 = (s[0]) ? ::atoi(&s[1]) : 0;
   const char* comma = strchr(s.c_str(), ',');
   const long arg2 = comma ? ::atoi(&comma[1]) : 0;
   switch (command) {
      /*g*/ case AComm::kScroll:          this->SendCmd(rCmdToggleScrollMode, arg1); break;
      /*t*/ case AComm::kTimer:           this->SendCmd(((AComm::kReset == arg1) ? rCmdTimerReset : rCmdToggleTimer), arg1); break;
      /*s*/ case AComm::kScrollSpeed:     this->SendMsg(AMswApp::sSetScrollSpeedMsg, arg1); break;
      /*p*/ case AComm::kScrollPos:       this->SendMsg(AMswApp::sSetScrollPosMsg, arg1); break;
      /*g*/ case AComm::kScrollPosSync:   this->SendMsg(AMswApp::sSetScrollPosSyncMsg, arg1); break;
      /*m*/ case AComm::kScrollMargins:   theApp.SetScrollMargins(arg1, arg2); break;
      /*c*/ case AComm::kCueMarker:       ARtfHelper::sCueMarker.SetPosition(-1, arg1); break;
      /*f*/ case AComm::kFrameInterval:   AScrollDialog::gMinFrameInterval = arg1; break;
   }
}

#include "utils.h"

void TRACE_LAST_ERROR(LPCSTR f, DWORD ln)
{
   DWORD err = GetLastError();
   CString etext = Utils::GetErrorText(err);
   CT2A ascii(etext);
   TRACE(("At %s line %d, err=%s\n"), f, ln, ascii.m_psz);
}

