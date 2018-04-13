#include "stdafx.h"
#include "PubnubComm.h"
#include "SendChannel.h"
#include "ReceiveChannel.h"
#include "PubnubCallback.h"
#include "Comm.h"
#include "strings.h"
#include "Msw.h"

#include "PubBufferTransfer.h" // for unit testing

#define PUBNUB_CALLBACK_API
extern "C" {
   void pn_printf(char* fmt, ...);

#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
#include "pubnub_version.h"
}
//#include <iostream>
#include <string>

#pragma comment (lib, "pubnub_callback")

namespace remchannel {
   // these are used by the channel interfaces
   const std::string JSON_PREFIX = std::string(1, (char)PREFIX);
   const std::string JSON_SUFFIX = std::string(1, (char)SUFFIX);
}

namespace {

   // channel naming convention
   const std::string channel_separator = "-"; // avoid Channel name disallowed characters
   const std::string app_name = "MSW"; // prefix for all channel names by convention

   bool RunUnitTests()
   {
      bool result = true;
      result &= PNBufferTransfer::UnitTest();
      return result;
   }
}

namespace {
   class RemoteIniFile {
      LPCTSTR secName;
      LPCTSTR fileName;

      // helpers
      static LPCTSTR GetFullPathOfExeFile();
      static LPCTSTR GetIniFilePath();
   public:
      RemoteIniFile(LPCTSTR fname_)
         : secName(_T("MSW_Pubnub"))
         , fileName(fname_)
      {
      }

      LPCTSTR GetFilePath();
      LPCTSTR GetValue( LPCTSTR keyName, LPCTSTR defValue = _T("") );
      const char* GetValueA( LPCTSTR keyName, LPCTSTR defValue = _T("") );
      void SetValue( LPCTSTR keyName, LPCTSTR newValue );
      void SetValueA( LPCTSTR keyName, LPCSTR newValue );
   };

   /*static*/ LPCTSTR RemoteIniFile::GetFullPathOfExeFile()
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

   /*static*/ LPCTSTR RemoteIniFile::GetIniFilePath()
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

   static LPCTSTR secName = _T("MSW_Pubnub");

   LPCTSTR RemoteIniFile::GetFilePath()
   {
      // NOTE: The params are TCHAR-style because file paths can include Unicode
      static std::wstring iniFilePath;
      iniFilePath = GetIniFilePath();
      iniFilePath += _T("msw.ini");
      LPCTSTR ini_path = iniFilePath.c_str();
      return ini_path;
   }

   LPCTSTR RemoteIniFile::GetValue( LPCTSTR keyName, LPCTSTR defValue )
   {
      static TCHAR buffer[1024];
      DWORD res;
      buffer[0] = '\0';
      res = ::GetPrivateProfileString(secName, keyName, defValue, buffer, sizeof(buffer), GetFilePath());
      TRACE(_T("CONFIG: %s returned %d: %s\n"), keyName, res, buffer);
      return buffer;
   }

   const char* RemoteIniFile::GetValueA( LPCTSTR keyName, LPCTSTR defValue )
   {
      LPCTSTR buffer = GetValue(keyName, defValue);
      CT2A ascii(buffer);
      static std::string s;
      s = ascii.m_psz; // remember lifetime issues, but use assignment
      return s.c_str();
   }

   void RemoteIniFile::SetValue( LPCTSTR keyName, LPCTSTR newValue )
   {
      LPCTSTR secName = _T("MSW_Pubnub");
      ::WritePrivateProfileStringW(secName, keyName, newValue, GetFilePath());
   }

   void RemoteIniFile::SetValueA( LPCTSTR keyName, LPCSTR newValue )
   {
      CA2T tcs(newValue);
      SetValue( keyName, tcs.m_psz);
   }

} // namespace (anonymous)

APubnubComm::APubnubComm(AComm* pComm) 
   : fComm(pComm)
   , fParent(nullptr)
   , fConnection(kNotSet)
   , fLinked(kDisconnected)
   , customerName("")
   , deviceName("")
   , deviceUUID("")
   , pubAPIKey("")
   , subAPIKey("")
   , fOperation(AComm::kNone)
   , statusCode(AComm::kSuccess)
   , statusMessage("")
   , cmdStartTime(0)
   , cmdLocalEndTime(0)
   , cmdRemoteEndTime(0)
   , cmdEndTime(0)
   , contactCode(AComm::kUnableToContact)
   , pReceiver(nullptr)
   , pSender(nullptr)
   , pBuffer(nullptr)
{
   //::InitializeCriticalSectionAndSpinCount(&cs, 0x400);
   //// spin count is how many loops to spin before actually waiting (on a multiprocesssor system)

   pReceiver = new ReceiveChannel(this); // receiver
   pSender = new SendChannel(this); // sender
   // we also need a buffer transfer object for coding the script for transfer
   pBuffer = new PNBufferTransfer();

   // emit build info
   TRACE("MSW Remote v0.1 built with Pubnub %s SDK v%s\n", pubnub_sdk_name(), pubnub_version());
   TRACE("Unit tests %s\n", RunUnitTests() ? "PASSED" : "FAILED");
}

APubnubComm::~APubnubComm(void)
{
   // shut down links too?!!?! but cannot fail or wait
   delete pSender;
   pSender = nullptr;
   delete pReceiver;
   pReceiver = nullptr;
   delete pBuffer;
   pBuffer = nullptr;
   //::DeleteCriticalSection(&cs);
}

void APubnubComm::Configure()
{
   /// NOTE: this exists so it can be called after the App object's registry key is set
   // determine connection type from parent's fMaster flag
   // NOTE: this will not change, unlike in early test code
   fConnection = fComm->IsMaster() ? kPrimary : kSecondary;

   // get scurrent ettings from persistent storage (registry)
   this->Read();

   bool changed = ReadOverrideFile("MSW.INI");

   // compose the local channel name from the device and company names
   std::string lchName = this->MakeChannelName(deviceName);

   // UUID: required for daily device tracking on PN network; do NOT look for this in INI override
   std::string uuid = this->deviceUUID;
   if (uuid.empty())
   {
      // generate and save a new unique ID for this instance in both channels
      struct Pubnub_UUID uuid_;
      char* random_uuid = nullptr;
      // docs and samples lied: v4 generator returns 0 if AOK, else -1 on error
      if (0 == pubnub_generate_uuid_v4_random(&uuid_)) {
         random_uuid = pubnub_uuid_to_string(&uuid_).uuid;
         //ofile.SetValueA(keyname_device_uuid, random_uuid); // don't write to the INI file either
         uuid = random_uuid;
      } else {
         uuid = lchName; // better than nothing? but might not be unique
      }
      this->deviceUUID = uuid;
      changed = true;
   }

   // write any changes back to the persistent store
   if (changed)
      this->Write();

   // set up the channels, once the settings are decided
   this->pReceiver->Setup(uuid, this->subAPIKey); // receiver only needs one key for listening
   this->pReceiver->SetName(lchName); // receiver knows channel name when local device name is known
   this->pSender->Setup(uuid, this->pubAPIKey, this->subAPIKey); // sender needs both keys but channel name comes later (dynamic)

   TRACE("Globally unique UUID for this device: %s\n", uuid.c_str());
}

namespace {
   // keep these implementation funcs in the anon.namespace
   void ReadRemoteProfileStringWithOverrideA(std::string& destSetting, LPCTSTR lpszEntry, LPCTSTR lpszDefault= _T(""))
   {
      CString s;
      s = theApp.GetProfileString(gStrComm, lpszEntry, lpszDefault);
      CT2A ascii = s;
      if (s != CString(lpszDefault)) {
         destSetting = ascii.m_psz;
      }
   }

   void WriteRemoteProfileStringA(LPCTSTR lpszEntry, const std::string& srcSetting)
   {
      CA2T unic(srcSetting.c_str());
      theApp.WriteProfileString(gStrComm, lpszEntry, unic.m_psz);
   }
}

void APubnubComm::Read()
{
   ReadRemoteProfileStringWithOverrideA(this->customerName, gStrCommCompany);
   ReadRemoteProfileStringWithOverrideA(this->deviceName, gStrCommDevice);
   ReadRemoteProfileStringWithOverrideA(this->deviceUUID, gStrCommDeviceUUID);
   ReadRemoteProfileStringWithOverrideA(this->pubAPIKey, gStrCommAPIPubkey);
   ReadRemoteProfileStringWithOverrideA(this->subAPIKey, gStrCommAPISubkey);
}

void APubnubComm::Write()
{
   WriteRemoteProfileStringA(gStrCommCompany, this->customerName);
   WriteRemoteProfileStringA(gStrCommDevice, this->deviceName);
   WriteRemoteProfileStringA(gStrCommDeviceUUID, this->deviceUUID);
   WriteRemoteProfileStringA(gStrCommAPIPubkey, this->pubAPIKey);
   WriteRemoteProfileStringA(gStrCommAPISubkey, this->subAPIKey);
}

bool APubnubComm::ReadOverrideFile(const char* fileName)
{
   CA2T unicode = fileName;
   RemoteIniFile ofile(unicode.m_psz);

   bool changed = false;
   // OPT FEATURE: import new settings from a private profile file in same place as EXE
   // USAGE: If file exists, its settings are examined for overrides to the above registry entries
   // NOTE: by using PrivateProfile with a specified full path, I can guarantee a file will be found if there
   // The registry is useful, but is much harder for the customer to edit when configuring their own accounts.
   // Some customers may prefer that, but this is provided as an alternate mechanism.
   LPCTSTR keyname_company = gStrCommCompany;
   LPCTSTR keyname_device = gStrCommDevice;
   //LPCTSTR keyname_device_uuid = gStrCommDeviceUUID; // this key is NOT read from INI file
   LPCTSTR keyname_pub = gStrCommAPIPubkey;
   LPCTSTR keyname_sub = gStrCommAPISubkey;

   std::string temps;
   temps = ofile.GetValueA(keyname_pub);
   if (!temps.empty())
      changed = true, this->pubAPIKey = temps;
   temps = ofile.GetValueA(keyname_sub);
   if (!temps.empty())
      changed = true, this->subAPIKey = temps;

   // load the company/customer and device name overrides
   temps = ofile.GetValueA(keyname_company);
   if (!temps.empty()) 
      changed = true, this->customerName = temps;

   temps = ofile.GetValueA(keyname_device);
   if (!temps.empty()) 
      changed = true, this->deviceName = temps;

   return changed;
}

void APubnubComm::SetParent(HWND parent)
{
   // SECONDARY: will forward all incoming messages to this window, after receipt of incoming data from Service
   fParent = parent;
}

#include <algorithm>
static std::string faulty = ".,:*/\\";
static char replacement = '=';
char replace_invalid(char input)
{
   std::string::size_type n;
   char result = input;
   n = faulty.find(input);
   if (n != std::string::npos)
      result = replacement; // invalid char WAS found, sub something else
   return result;
}

std::string RemoveInvalidCharacters( std::string& s )
{
   std::transform(s.begin(), s.end(), s.begin(), replace_invalid);
   return s;
}

std::string APubnubComm::MakeChannelName( const std::string& deviceName ) const
{
   std::string result;
   result += app_name;
   result += channel_separator;
   result += (this->customerName);
   result += channel_separator;
   result += deviceName;
   RemoveInvalidCharacters(result);
   // unit test
   TRACE("Unit test of IsSameCompany(%s) %s\n", result.c_str(), IsSameCompany(result) ? "PASSED" : "FAILED");
   return result;
}

bool APubnubComm::IsSameCompany( const std::string& channelName ) const
{
   bool result = true;
   std::vector<std::string> sa = rem::split( channelName.c_str(), channel_separator.c_str() );
   if (sa.size() == 3)
      result = this->customerName == sa[1];
   return result;
}

const char* APubnubComm::GetConnectionTypeName() const
{
   return rem::GetConnectionTypeName(this->fConnection);
}

const char* APubnubComm::GetOperationName() const
{
   const char* result = "UNKNOWN";
   switch(this->fOperation) {
   case AComm::kNone: result = "NONE"; break;
   case AComm::kLogin: result = "LOGIN"; break;
   case AComm::kLogout: result = "LOGOUT"; break;
   case AComm::kConnect: result = "CONNECT"; break;
   case AComm::kDisconnect: result = "DISCONNECT"; break;
   case AComm::kScrollOn: result = "SCROLL-ON"; break;
   case AComm::kScrollOff: result = "SCROLL-OFF"; break;
   case AComm::kContact: result = "CONTACT"; break;
   case AComm::kContactCancel: result = "UNCONTACT"; break;
   case AComm::kFileSend: result = "SENDDATA"; break;
   case AComm::kFileReceive: result = "RCVDATA"; break;
   case AComm::kFileCancel: result = "CANDATA"; break;
   }
   return result;
}

const char* APubnubComm::GetConnectionStateName() const
{
   return rem::GetConnectionStateName(this->fLinked);
}

namespace rem {

   const char* GetConnectionTypeName( ConnectionType type )
   {
      const char* result = "UNKNOWN";
      switch (type) {
      case kNotSet: result = "UNCONFIGURED"; break;
      case kPrimary: result = "PRIMARY"; break;
      case kSecondary: result = "SECONDARY"; break;
      case kNoChange: result = "NO CHANGE"; break;
      }
      return result;
   }

   const char* GetConnectionStateName( ConnectionStatus status )
   {
      const char* result = "UNKNOWN";
      switch (status) {
      case kDisconnected: result = "OFFLINE"; break;
      case kDisconnecting: result = "DISCONNECTING"; break;
      case kConnecting: result = "CONNECTING"; break;
      case kConnected: result = "ONLINE"; break;
      case kLinking: result = "PAIRING"; break;
      case kUnlinking: result = "UNPAIRING"; break;
      case kChatting: result = "PAIRED"; break;
      case kScrolling: result = "SCROLLING"; break;
      case kFileSending: result = "FILE SENDING"; break;
      case kFileRcving: result = "FILE RECEIVING"; break;
      case kFileCanceling: result = "CANCELING FILE"; break;
      case kBusy: result = "IN TRANSACTION"; break;
      }
      return result;
   }

} // namespace rem

namespace remchannel {

   const char* GetTypeName( type t )
   {
      const char* result = "UNKNOWN";
      switch (t) {
      case kReceiver: result = "RECEIVER"; break;
      case kSender: result = "SENDER"; break;
      }
      return result;
   }

   const char* GetStateName( state s )
   {
      const char* result = "Unknown";
      switch (s) {
      case kNone: result = "None"; break;
      case kDisconnected: result = "Disconnected"; break;
      case kDisconnecting: result = "Disconnecting"; break;
      case kConnecting: result = "Connecting"; break;
      case kIdle: result = "Idle"; break;
      case kBusy: result = "Busy"; break;
      }
      return result;
   }

} // namespace rem

// function to capture pubnub_log output to the TRACE() window (ONLY FOR OUR CODE)
#define PNC_MESSAGE_LOG(...) this->StoreMessage(false, __VA_ARGS__)
#define PNC_MESSAGE_LOGX(...) this->StoreMessage(true, __VA_ARGS__)
void APubnubComm::StoreMessage(bool clear, char* fmt, ...)
{
   const int bufsize = 10240;
   static char buffer[bufsize];

   va_list args;
   va_start(args,fmt);
   vsprintf_s(buffer,bufsize,fmt,args);
   va_end(args);

   if (clear)
      this->statusMessage = buffer;
   else
      this->statusMessage += buffer;

   TRACE("%s\n", buffer);
}

bool APubnubComm::isBusy() const
{
   bool result = false;
   switch (this->fLinked) {
   case kConnecting:
   case kDisconnecting:
   case kLinking:
   case kUnlinking:
   case kBusy:
      result = true;
      break;
   default:
      break;
   }
   return result;
}

CString APubnubComm::GetConnectedName() const
{
   // need to return the same name as listed in the UI dialog box or the fUsername in the AComm::slaves member
   CA2T tChannel( pSender->GetDeviceName() );
   return tChannel.m_psz;
}

int APubnubComm::GetStatusCode() const
{
   return this->statusCode;
}

int APubnubComm::GetContactCode() const
{
   return this->contactCode;
}

bool APubnubComm::isSuccessful() const
{
   return this->statusCode == AComm::kSuccess;
}

double APubnubComm::GetCommandTimeSecs() const
{
   // NOTE: rem::get_local_timestamp() returns a high-resolution time in 100ns intervals (10^-7 sec)
   double result = difftime(cmdEndTime, cmdStartTime); // bigger one first gives positive diffs, same as - op
   result /= 1e7; // convert to secs (standard rep)
   return result;
}

// This will finalize any of the four major busy operations, and report any state change to the parent, even those externally set by caller
void APubnubComm::OnTransactionComplete(remchannel::type which, remchannel::result what)
{
   time_t timestamp = rem::get_local_timestamp();
   switch(fLinked) {
   case kConnecting:
      // Logging in
      if (what == remchannel::kOK)
         this->SetState( kConnected, AComm::kSuccess );
      else
         this->SetState( kDisconnected, AComm::kUnableToLogin );
      break;
   case kDisconnecting:
      // Logging out
      // async logout still can only happen on receiver, since it is the only one logged in
      if (what == remchannel::kOK)
         this->SetState( kDisconnected, AComm::kSuccess );
      else
         this->SetState( kConnected, AComm::kUnableToLogout );
      break;
   case kLinking:
      // Pairing/connecting
      // async connects can only happen for senders, and only when they are in a publish transaction
      if (what == remchannel::kOK)
         this->SetState( kChatting, AComm::kSuccess );
      else
         this->SetState( kConnected, AComm::kUnableToPair );
      break;
   case kUnlinking:
      // Unpairing/disconnecting
      // async logout still can only happen on receiver, since it is the only one logged in
      if (what == remchannel::kOK)
         this->SetState( kConnected, AComm::kSuccess );
      else
         this->SetState( kChatting, AComm::kUnableToUnpair );
      break;
   case kBusy:
      switch (fOperation) {
      case AComm::kScrollOn:
         // Start scrolling
         if (what == remchannel::kOK)
            this->SetState( kScrolling, AComm::kSuccess );
         else
            this->SetState( kChatting, AComm::kUnableToStartScroll );
         break;
      case AComm::kScrollOff:
         // Stop scrolling
         if (what == remchannel::kOK)
            this->SetState( kChatting, AComm::kSuccess );
         else
            this->SetState( kScrolling, AComm::kUnableToStopScroll );
         break;
      case AComm::kContact:
      case AComm::kContactCancel:
      //case kOtherRoundTripOperation: // probably kFileXXX
         // for all op-busy-wait-reply scenarios (list above)
         // check who this is from, ignore the Sender if OK, wait for the Receiver to complete, catch errors
         if (which == remchannel::kSender && what == remchannel::kError) {
            // error in sender publish transaction
            this->cmdLocalEndTime = timestamp; // remember time split for local
            this->contactCode = AComm::kUnableToSendCommand;
            this->SetState( kChatting, this->contactCode );
         } else if (which == remchannel::kReceiver && what == remchannel::kError) {
            // error in receiver subscribe response (timeout, etc.)
            this->contactCode = AComm::kUnableToGetResponse;
            this->SetState( kChatting, this->contactCode );
         } else if (which == remchannel::kReceiver) {
            // receiver got a response with data (dispatched call by this point)
            // The response code number is now stored in the contactCode, and other data may also be stored
            this->SetState( kChatting, this->contactCode );
         } else {
            // sender sent OK - intermediate step, don't fire to next level
            this->cmdLocalEndTime = timestamp; // remember time split for local
            this->cmdEndTime = timestamp; // update overall time
            PNC_MESSAGE_LOG("%s %s SENT %s OP SUCCESSFULLY ON CHANNEL %s @ %1.6lf s\n", this->GetConnectionTypeName(), 
               pSender->GetTypeName(), this->GetOperationName(), pSender->GetName(), this->GetCommandTimeSecs());
            return;
         }
         break;
      }
      break;
   }

   // save the overall transaction end time
   this->cmdEndTime = timestamp;

   // log state change
   const char* tname = (which == remchannel::kReceiver) ? pReceiver->GetTypeName() : pSender->GetTypeName();
   const char* cname = (which == remchannel::kReceiver) ? pReceiver->GetName() : pSender->GetName();
   PNC_MESSAGE_LOG("%s %s ENDED %s OP WITH %s ON CHANNEL %s @ %1.6lf s\n", this->GetConnectionTypeName(), tname, 
      this->GetOperationName(), this->GetConnectionStateName(), cname, this->GetCommandTimeSecs());

   // fire state change up to the next level
   this->SendStatusReport();
}

void APubnubComm::SendStatusReport() const
{
   // to change its own state and post a status message to the UI
   fComm->OnStateChange( (AComm::OpType)fOperation, (AComm::Status)statusCode ); 
}

bool APubnubComm::Login(const char* ourDeviceName_)
{
   fOperation = AComm::kLogin;
   remchannel::type who = remchannel::kReceiver;
   PNC_MESSAGE_LOGX("");
   // save when we started the transaction
   cmdStartTime = rem::get_local_timestamp();
   // report where we are each time
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pReceiver->GetTypeName(), pReceiver->GetName());
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pSender->GetTypeName(), pSender->GetName());

   if (fLinked > kDisconnected) {
      PNC_MESSAGE_LOG("%s %s REQUESTED, ALREADY %s.\n", this->GetConnectionTypeName(), this->GetOperationName(), 
         (fLinked == kConnected) ? "LOGGED IN" : "LOGGING IN");
      // don't change statusCode or state here, just notify request completion
      //this->SetState( this->fLinked, this->statusCode );
      this->OnTransactionComplete( who, remchannel::kOK );
      return true;
   }

   if (fConnection == kNotSet) {
      PNC_MESSAGE_LOG("DEVICE IS %s, UNABLE TO OPEN CHANNEL LINK.\n", this->GetConnectionTypeName());
      this->SetState( this->fLinked, AComm::kUnconfigured );
      this->OnTransactionComplete( who, remchannel::kError );
      return false;
   }

   std::string ourDeviceName = ourDeviceName_ ? ourDeviceName_ : "";
   // if we've been given a new name, change our channel name here
   if (!ourDeviceName.empty()) {
      this->deviceName = ourDeviceName;
      this->Write();
      pReceiver->SetName( this->MakeChannelName(ourDeviceName) );
   }

   bool result = true;
   // PRIMARY:
   if (this->isPrimary()) {
      // TBD - get proxy info if needed using special proxy thread and context (Issue #10-B)
      // NOTE: To save message traffic, Primary will only listen when a transaciton is in progress that needs it
      PNC_MESSAGE_LOG("%s IS ONLINE AND READY.\n", this->GetConnectionTypeName());
      this->SetState( kConnected );
      this->OnTransactionComplete( who, remchannel::kOK );
      return result;
   }

   // SECONDARY:
   if (fLinked >= kConnected) {
      PNC_MESSAGE_LOG("%s %s, LINK ALREADY LOGGED IN.\n", this->GetConnectionTypeName(), this->GetOperationName());
      // don't change state here
      //this->SetState( this->fLinked, this->statusCode );
      this->OnTransactionComplete( who, remchannel::kOK );
      return result;
   }

   if (this->pReceiver->isUnnamed()) {
      PNC_MESSAGE_LOG("%s %s: %s HAS NO CHANNEL NAME, UNABLE TO OPEN CHANNEL LINK.\n", this->GetConnectionTypeName(), this->GetOperationName(), pReceiver->GetTypeName());
      this->SetState( this->fLinked, AComm::kNoChannelName );
      this->OnTransactionComplete( who, remchannel::kError );
      return false;
   }

   // set up receiver to listen on our private channel
   // NOTE: callback can happen immediately, so set state first; callback will do state transitions
   this->SetState( kConnecting, AComm::kWaiting );
   if (!(pReceiver->Init())) {
      // immediate failure
      PNC_MESSAGE_LOG("%s %s IS UNABLE TO CONNECT TO THE NET ON CHANNEL %s (STATE=%d)\n", 
         this->GetConnectionTypeName(), ourDeviceName_, pReceiver->GetName(), fLinked);

      this->OnTransactionComplete(who, remchannel::kError);
      result = false;
   } else if (pReceiver->IsBusy()) {
      // async completion
      PNC_MESSAGE_LOG("%s LOGIN: %s IS WAITING TO LISTEN AS %s ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), pReceiver->GetTypeName(), ourDeviceName_, pReceiver->GetName());

   } else {
      // immediate completion
      PNC_MESSAGE_LOG("%s LOGIN: %s IS LISTENING AS %s ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), pReceiver->GetTypeName(), ourDeviceName_, pReceiver->GetName());

      this->OnTransactionComplete(who, remchannel::kOK);
   }
   return result; // started sequence successfully
}

void APubnubComm::Logout()
{
   fOperation = AComm::kLogout;
   remchannel::type who = remchannel::kReceiver;
   PNC_MESSAGE_LOGX("");
   // save when we started the transaction
   cmdStartTime = rem::get_local_timestamp();

   // report where we are each time
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pReceiver->GetTypeName(), pReceiver->GetName());
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pSender->GetTypeName(), pSender->GetName());

   if (fLinked < kConnected) {
      PNC_MESSAGE_LOG("%s LOGOUT REQUESTED, ALREADY %s OUT.\n", this->GetConnectionTypeName(), 
         (fLinked == kDisconnected) ? "LOGGED" : "LOGGING");
      //this->SetState( this->fLinked, this->statusCode );
      this->OnTransactionComplete(who, remchannel::kOK);
      return;
   }

   if (pReceiver->isUnnamed()) {
      PNC_MESSAGE_LOG("%s HAS NO %s LINK TO SHUT DOWN.\n", this->GetConnectionTypeName(), pReceiver->GetTypeName());
      this->SetState( this->fLinked, AComm::kNoChannelName );
      this->OnTransactionComplete(who, remchannel::kError);
      return;
   }

   // shut down any pSender link first
   //this->CloseLink();
   if (fLinked > kConnected) {
      PNC_MESSAGE_LOG("%s LOGOUT ERROR: %s LINK STILL OPERATING, DISCONNECT FIRST.\n", this->GetConnectionTypeName(), pReceiver->GetTypeName());
      //this->SetState( this->fLinked, this->statusCode );
      this->OnTransactionComplete(who, remchannel::kError); // TBD - need to send status code w/o changing existing one
      return;
   }

   //TRACE("%s IS SHUTTING DOWN %s LINK TO PUBNUB CHANNEL %s\n", this->GetConnectionTypeName(), pReceiver->GetTypeName(), pReceiver->GetName());

   // make it so 
   // PRIMARY:
   if (this->isPrimary()) {
      // NOTE: We do not use Primary at login time, so nothing to log out.
      PNC_MESSAGE_LOG("%s IS OFFLINE.\n", this->GetConnectionTypeName());
      this->SetState( kDisconnected, AComm::kSuccess );
      this->OnTransactionComplete(who, remchannel::kOK);
      return;
   }

   // SECONDARY: shut down the listener loop in Receiver channel
   // NOTE: If this is async, we need to be told to wait.
   this->SetState( kDisconnecting, AComm::kWaiting );
   bool res = pReceiver->DeInit();
   bool busy = pReceiver->IsBusy();
   const char* chname = pReceiver->GetName();
   if (!res) {
      // immediate failure
      PNC_MESSAGE_LOG("%s IS UNABLE TO SHUT DOWN CHANNEL %s\n", 
         this->GetConnectionTypeName(), chname);
      this->OnTransactionComplete(who, remchannel::kError);
      // TBD - but this really will cause problems! what are failure scenarios here?
   } else if (busy) {
      // waiting to find out results
      PNC_MESSAGE_LOG("%s WAITING TO SHUT DOWN CHANNEL %s\n", 
         this->GetConnectionTypeName(), chname);
   } else {
      // immediate success
      PNC_MESSAGE_LOG("%s CORRECTLY SHUT DOWN CHANNEL %s\n", 
         this->GetConnectionTypeName(), chname);
      this->OnTransactionComplete(who, remchannel::kOK);
   }
}

bool APubnubComm::OpenLink(const char * pSenderName_)
{
   fOperation = AComm::kConnect;
   remchannel::type who = remchannel::kReceiver;
   PNC_MESSAGE_LOGX("");
   // save when we started the transaction
   cmdStartTime = rem::get_local_timestamp();

   // report where we are each time
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pReceiver->GetTypeName(), pReceiver->GetName());
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pSender->GetTypeName(), pSender->GetName());

   if (fLinked == kLinking || fLinked == kChatting) {
      PNC_MESSAGE_LOG("%s CONNECT REQUESTED, ALREADY CONNECT%s.\n", this->GetConnectionTypeName(), 
         (fLinked == kChatting) ? "ED" : "ING");
      // don't change statusCode or state here
      this->OnTransactionComplete(who, remchannel::kOK);
      return true;
   }

   bool result = true;
   // This is where we need to set up the Sender for publishing (Init() call)
   // OPEN LINK: state is kChatting
   // TRANSITION STATES: turning on = kLinking, turning off = kUnlinking
   // CLOSED LINK: state is kConnected

   // SECONDARY - could happen over the link; any need for different behavior here?

   // PRIMARY: make sure the sender can operate to send messages
   if (fLinked >= kChatting) {
      PNC_MESSAGE_LOG("%s CONNECT, LINK ALREADY PAIRED TO CHANNEL %s.\n", this->GetConnectionTypeName(), pSender->GetName());
      // don't change state here
      this->OnTransactionComplete(who, remchannel::kOK);
      return result;
   }

   this->SetState( kLinking, AComm::kWaiting );
   std::string secondaryName = this->MakeChannelName(pSenderName_);
   pSender->SetDeviceName(pSenderName_);
   bool res = pSender->Init( secondaryName );
   bool busy = pSender->IsBusy();
   const char* chname = pSender->GetName();
   const char* tname = pSender->GetTypeName();
   if (!res) {
      // immediate failure
      PNC_MESSAGE_LOG("%s %s IS UNABLE TO SETUP PAIRED LINK TO CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
      
      this->OnTransactionComplete(remchannel::kSender, remchannel::kError);
   } else if (busy) {
      // waiting to find out results
      PNC_MESSAGE_LOG("%s %s WAITING TO SET UP PAIRED LINK TO CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
   } else {
      // immediate success
      PNC_MESSAGE_LOG("%s %s CORRECTLY SET UP PAIRED LINK TO CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
      this->OnTransactionComplete(remchannel::kSender, remchannel::kOK);
   }

   return result; // started sequence successfully
}

void APubnubComm::CloseLink()
{
   fOperation = AComm::kDisconnect;
   remchannel::type who = remchannel::kSender;
   PNC_MESSAGE_LOGX("");
   // save when we started the transaction
   cmdStartTime = rem::get_local_timestamp();

   // report where we are each time
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pReceiver->GetTypeName(), pReceiver->GetName());
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pSender->GetTypeName(), pSender->GetName());

   if (fLinked < kChatting) {
      PNC_MESSAGE_LOG("%s %s REQUESTED, ALREADY %s%s.\n", this->GetConnectionTypeName(), this->GetOperationName(), this->GetOperationName(), 
         (fLinked == kConnected) ? "ED" : "ING");
      this->OnTransactionComplete(who, remchannel::kOK);
      return; // already closed
   }

   // shut down any higher states here (Scrolling, FileSend, Busy transactions w PQ)
   if (fLinked > kChatting) {
      PNC_MESSAGE_LOG("%s DISCONNECT ERROR: %s LINK STILL OPERATING, SHUTTING DOWN.\n", this->GetConnectionTypeName(), pReceiver->GetTypeName());
      this->OnTransactionComplete(who, remchannel::kError);
      return;
   }

   // OPEN LINK: state is kChatting
   // TRANSITION STATES: turning on = kLinking, turning off = kUnlinking
   // CLOSED LINK: state is kConnected

   // SECONDARY - this will be started by command over the link - any special behavior TBD

   // PRIMARY: shut down the publishing loop, if any, and cancel any transaction in progress
   this->SetState( kUnlinking, AComm::kWaiting );
   bool res = pSender->DeInit();
   bool busy = pSender->IsBusy();
   const char* chname = pSender->GetName();
   const char* tname = pSender->GetTypeName();
   if (!res) {
      // immediate failure
      PNC_MESSAGE_LOG("%s %s IS UNABLE TO SHUT DOWN PAIRED LINK TO CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
      
      this->OnTransactionComplete(remchannel::kSender, remchannel::kError);
   } else if (busy) {
      // waiting to find out results
      PNC_MESSAGE_LOG("%s %s WAITING TO SHUT DOWN PAIRED LINK TO CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
   } else {
      // immediate success
      PNC_MESSAGE_LOG("%s %s CORRECTLY SHUT DOWN PAIRED LINK TO CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
      this->OnTransactionComplete(remchannel::kSender, remchannel::kOK);
   }
}


bool APubnubComm::StartScrollMode()
{
   fOperation = AComm::kScrollOn;
   remchannel::type who = remchannel::kReceiver;
   PNC_MESSAGE_LOGX("");
   // save when we started the transaction
   cmdStartTime = rem::get_local_timestamp();

   // report where we are each time
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pReceiver->GetTypeName(), pReceiver->GetName());
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pSender->GetTypeName(), pSender->GetName());

   if (fLinked >= kScrolling || (fLinked == kBusy && fOperation == AComm::kScrollOn)) {
      PNC_MESSAGE_LOG("%s %s REQUESTED, ALREADY %s.\n", this->GetConnectionTypeName(), this->GetOperationName(), 
         (fLinked == kBusy) ? "DONE" : "IN PROGRESS");
      // don't change statusCode or state here
      this->OnTransactionComplete(who, remchannel::kOK);
      return true;
   }

   bool result = true;
   // This is where we need to set up the Sender for publishing (Init() call)
   // STARTED SCROLLING: state is kScrolling
   // TRANSITION STATES: turning on or off = kBusy (check fOperation)
   // STOPPED SCROLLING: state is kChatting

   // PRIMARY: make sure the sender can operate to receive response messages
   this->SetState( kBusy, AComm::kWaiting );
   bool res = pReceiver->Init();
   bool busy = pReceiver->IsBusy();
   const char* chname = pReceiver->GetName();
   const char* tname = pReceiver->GetTypeName();
   if (!res) {
      // immediate failure
      PNC_MESSAGE_LOG("%s %s IS UNABLE TO SETUP SCROLL LISTENER ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
      
      this->OnTransactionComplete(who, remchannel::kError);
      result = false;
   } else if (busy) {
      // waiting to find out results
      PNC_MESSAGE_LOG("%s %s WAITING TO SET UP SCROLL LISTENER ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
   } else {
      // immediate success
      PNC_MESSAGE_LOG("%s %s CORRECTLY SET UP SCROLL LISTENER ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
      this->OnTransactionComplete(who, remchannel::kOK);
   }

   return result; // started sequence successfully
}

void APubnubComm::StopScrollMode()
{
   fOperation = AComm::kScrollOff;
   remchannel::type who = remchannel::kReceiver;
   PNC_MESSAGE_LOGX("");
   // save when we started the transaction
   cmdStartTime = rem::get_local_timestamp();

   // report where we are each time
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pReceiver->GetTypeName(), pReceiver->GetName());
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pSender->GetTypeName(), pSender->GetName());

   if (fLinked == kChatting || (fLinked == kBusy && fOperation == AComm::kScrollOff)) {
      PNC_MESSAGE_LOG("%s %s REQUESTED, ALREADY %s.\n", this->GetConnectionTypeName(), this->GetOperationName(), 
         (fLinked == kConnected) ? "DONE" : "IN PROGRESS");
      this->OnTransactionComplete(who, remchannel::kOK);
      return; // already closed
   }

   // PRIMARY: Shut down the Receiver listen loop
   this->SetState( kBusy, AComm::kWaiting );
   bool res = pReceiver->DeInit();
   bool busy = pReceiver->IsBusy();
   const char* chname = pReceiver->GetName();
   const char* tname = pReceiver->GetTypeName();
   if (!res) {
      // immediate failure
      PNC_MESSAGE_LOG("%s %s IS UNABLE TO SHUT DOWN SCROLL LISTENER ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
      
      this->OnTransactionComplete(who, remchannel::kError);
   } else if (busy) {
      // waiting to find out results
      PNC_MESSAGE_LOG("%s %s WAITING TO SHUT DOWN SCROLL LISTENER ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
   } else {
      // immediate success
      PNC_MESSAGE_LOG("%s %s CORRECTLY SHUT DOWN SCROLL LISTENER ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, chname);
      this->OnTransactionComplete(who, remchannel::kOK);
   }
}

void APubnubComm::SendCommand(const char * message)
{
   //bool result = true;
   // don't allow if not in paired conversation mode
   if (fLinked < kChatting) {
      //
      TRACE("MESSAGE BLOCKED: %s\n", message);
      return;
   }

   // publish message to pSender->channelName and set up callback
   // PRIMARY: will send commands
   // SECONDARY: will send any responses
   std::string msg = (message);
   TRACE("SENDING MESSAGE %s:", msg.c_str()); // DEBUGGING
   if (false == pSender->Send(message)) {
      TRACE(pSender->GetLastMessage());
   }
   TRACE("\n");
}

bool APubnubComm::SendCommandBusy(int op)
{
   fOperation = op;
   remchannel::type who = remchannel::kReceiver;
   PNC_MESSAGE_LOGX("");
   // save when we started the transaction
   cmdStartTime = rem::get_local_timestamp();
   const unsigned int WAIT_TIME_SECS = 30;

   // report where we are each time
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pReceiver->GetTypeName(), pReceiver->GetName());
   TRACE("%s STATE=%s FOR %s %s ON %s\n", this->GetOperationName(), this->GetConnectionStateName(), this->GetConnectionTypeName(), pSender->GetTypeName(), pSender->GetName());

   if (fLinked == kBusy && fOperation == op) {
      PNC_MESSAGE_LOG("%s %s REQUESTED, ALREADY %s.\n", this->GetConnectionTypeName(), this->GetOperationName(), 
         (fLinked == kBusy) ? "DONE" : "IN PROGRESS");
      // don't change statusCode or state here
      this->OnTransactionComplete(who, remchannel::kOK);
      return true;
   }

   bool result = true;
   if (fLinked < kChatting) {
      PNC_MESSAGE_LOG("%s %s REQUESTED, CANNOT START IN STATE %s.\n", this->GetConnectionTypeName(), this->GetOperationName(), 
         this->GetConnectionStateName());
      this->SetState( this->fLinked, AComm::kUnableToContact );
      this->OnTransactionComplete(who, remchannel::kError);
      return false;
   }

   // PRIMARY: make sure the sender can operate to receive response messages
   this->SetState( kBusy, AComm::kWaiting );
   bool res = pReceiver->Listen(WAIT_TIME_SECS); // special single non-loop version of subscribe, specifies wait in secs
   bool busy = pReceiver->IsBusy();
   const char* chname = pReceiver->GetName();
   const char* tname = pReceiver->GetTypeName();
   if (!res) {
      // immediate failure of Listen() -- check error codes
      PNC_MESSAGE_LOG("%s %s IS UNABLE TO WAIT FOR CMD %s ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, this->GetOperationName(), chname);
      this->contactCode = AComm::kUnableToGetResponse;
      this->OnTransactionComplete(who, remchannel::kError);
      result = false;
   } else if (busy) {
      // waiting to find out results, OK to proceed with send
      PNC_MESSAGE_LOG("%s %s SETTING UP RESPONSE LISTENER FOR %s CMD ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, this->GetOperationName(), chname);
      // now send the op-related message to the sender, which is configured if we're kChatting
      const char* cmd = this->FormatOperation( op );
      // send the command that will trigger the response eventually
      bool res = pSender->Send(cmd);
      if (!res) {
         // can't send command? reject the operation here
         this->contactCode = AComm::kUnableToSendCommand;
         this->OnTransactionComplete(remchannel::kSender, remchannel::kError);
      } else {
         // send started OK? now we wait for the replies to come back from the Sender and Receiver
         // proper end will be from the Receiver reply which SHOULD call OTC() after the Sender reply comes in
         // we need to continue after the Sender reply, which will also call OTC(), and deal with out-of-order arrival
         this->contactCode = AComm::kWaiting;
      }
   } else {
      // immediate success - also not an option, can't have a reply before we send, can we? but stale replies ...
      PNC_MESSAGE_LOG("%s %s PREMATURE RETURN FROM LISTEN REPLY TO CMD %s ON CHANNEL %s\n", 
         this->GetConnectionTypeName(), tname, this->GetOperationName(), chname);
      this->contactCode = AComm::kUnableToGetResponse;
      this->OnTransactionComplete(who, remchannel::kError);
   }

   return result; // started sequence successfully
}

const char* APubnubComm::FormatOperation(int opCode)
{
   const char* cmd = nullptr;
   switch(opCode) {
   case AComm::kContact:
      cmd = this->FormatCommand( AComm::kContact, AComm::kOn, 0, pReceiver->GetName() );
      break;
   case AComm::kContactCancel:
      cmd = this->FormatCommand( AComm::kContactCancel, AComm::kOff, 0, pReceiver->GetName() );
      break;
   }
   return cmd;
}

const char* APubnubComm::FormatCommand( int opCode, int arg1, int arg2, const std::string& argS )
{
   static char buffer[256];
   switch (opCode) {
   case AComm::kContact:
   case AComm::kContactCancel:
      sprintf_s(buffer, 256, "%c%d,%s", AComm::kContactRemote, arg1, argS.c_str());
      break;
   default:
      buffer[0] = 0;
      break;
   }
   return buffer;
}

void APubnubComm::OnContactMessage(int onOff, const std::string& channel_name )
{
   TRACE("%s Received Contact Command: %d,%s\n", this->GetConnectionTypeName(), onOff, channel_name.c_str());

   if (this->isSecondary()) {
      // Scenario: receiving the full command including channel name from the Primary
      bool differentChannel = (channel_name != pSender->GetName());
      AComm::Status reason = AComm::kSuccess;
      ConnectionStatus nextState = this->fLinked; // what state to change to ultimately
      bool accepted = true;
      const char* chname = pSender->GetName();
      const char* ovr_channel = differentChannel ? channel_name.c_str() : nullptr;
      switch (onOff) {
      case AComm::kOn:
         // CONTACT COMMAND w. channel name
         TRACE("%s Received CONTACT-ON from channel %s.\n", this->GetConnectionTypeName(), channel_name.c_str());
         // secondary machine receives primary's chanel name
         // check if we are not in proper state to be listening (huh? how??)
         // check if already have a conversation and reject a new one if for a different channel
         if (this->fLinked == kChatting && differentChannel) {
            TRACE("%s CANNOT ACCEPT CONNECT REQUEST, ALREADY IN CONVERSATION WITH %s\n", this->GetConnectionTypeName(), chname);
            // remember to send this channel name back in the reply
            accepted = false, reason = AComm::kContactRejected;
         }
         // else verify channel name and reject if badly formatted, wrong company name, bad onOff command
         else if (!this->IsSameCompany( channel_name ))
            accepted = false, reason = AComm::kOutsideDomain;
         else
            reason = AComm::kSuccess;
         // else proceed with ACCEPT or REJECT
         if (accepted) {
            // set kChatting state if we are now in OK conversation
            nextState = kChatting;
            // set up the Sender channel with the Primary name if OK (extract device name too?)
            pSender->Init( channel_name );
         } else if (reason == AComm::kContactRejected) {
            // SPECIAL CASE: we need to send the command to a channel different than what we are currently sending on
            TRACE("%s SENDING REJECTION MESSAGE TO CHANNEL %s WHILE TALKING ON CHANNEL %s\n", 
               this->GetConnectionTypeName(), channel_name.c_str(), pSender->GetName() );
            // no state change needed
         }
         // else no state change needed, reason already set
         break;
      case AComm::kOff:
         // CANCEL-CONTACT COMMAND w. channel name
         // verify if the channel is one we are talking to, and we are actually talking
         // ignore other combinations (reject command silently for now - log it tho)
         if (this->fLinked == kChatting && !differentChannel) {
            reason = AComm::kSuccess;
            nextState = kConnected;
            TRACE("%s Received CONTACT-CANCEL for channel %s.\n", this->GetConnectionTypeName(), channel_name.c_str());
            pSender->DeInit();
            if (pSender->IsBusy()) {
               // NOTE: this needs to wait and could fail if pubnub can't cancel; is this worth noting?
               reason = AComm::kWaiting; 
               nextState = kUnlinking;
            }
         } else {
            reason = differentChannel ? AComm::kAlreadyInProgress : AComm::kUnableToUnpair;
            TRACE("%s Ignored CONTACT-CANCEL for channel %s in state %s.\n", 
               this->GetConnectionTypeName(), channel_name.c_str(), rem::GetConnectionStateName(this->fLinked));
            // no state change
         }
         break;
      default:
         reason = AComm::kBadFormat;
         // no state change
         TRACE("%s Received BAD CODE %d for channel %s.\n", this->GetConnectionTypeName(), onOff, channel_name.c_str());
         break;
      }
      // Secondary will always send the response code in a response message
      // NOTE: this is sent in response to data; there is no transaction in progress until this comes in
      // however, the Send channel is now busy while the response is being sent, so make sure it has a finalization sequence
      const char* cmd = this->FormatCommand(AComm::kContact, reason );
      bool res = pSender->Send(cmd, ovr_channel);
      this->contactCode = reason;
      this->SetState( nextState, reason );
      if (!res) {
         TRACE("UNABLE TO SEND REPLY TO CONTACT CMD BACK TO PRIMARY (PUB ERROR %s)\n", pSender->GetLastMessage());
      }

   } else if (this->isPrimary()) {
      // primary receives secondary's reply (sends actual rejection code number)
      // set contactCode according to final status (onOff)
      // able to finally complete round-trip operation now
      this->contactCode = onOff;
      TRACE("Received REASON CODE %d from Secondary on channel %s.\n", onOff, channel_name.c_str());
      // NOTE: transaction should be finished elsewhere, this is only the response to the command data
   }
}

#include "Msw.h"
#include "RtfHelper.h"
#include "ScrollDialog.h"

void APubnubComm::OnMessage(const char * message)
{
   //bool result = true;
   TRACE("RECEIVED MESSAGE ON THREAD 0x%X:%s\n", ::GetCurrentThreadId(), message); // DEBUGGING
   // this is the callback for messages received on pReceiver->channelName
   // PRIMARY: will receive any responses
   // SECONDARY: will receive commands
   std::string s = message;
   const char command = s[0];
   const long arg1 = (s[0]) ? ::atoi(&s[1]) : 0;
   const char* comma = strchr(s.c_str(), ',');
   const long arg2 = comma ? ::atoi(&comma[1]) : 0;
   const std::string arg2S = comma ? &comma[1] : "";
   switch (command) {
      case AComm::kScroll:          /*g*/ this->SendCmd(rCmdToggleScrollMode, arg1); break;
      case AComm::kTimer:           /*t*/ this->SendCmd(((AComm::kReset == arg1) ? rCmdTimerReset : rCmdToggleTimer), arg1); break;
      case AComm::kScrollSpeed:     /*s*/ this->SendMsg(AMswApp::sSetScrollSpeedMsg, arg1); break;
      case AComm::kScrollPos:       /*p*/ this->SendMsg(AMswApp::sSetScrollPosMsg, arg1); break;
      case AComm::kScrollPosSync:   /*y*/ this->SendMsg(AMswApp::sSetScrollPosSyncMsg, arg1); break;
      case AComm::kScrollMargins:   /*m*/ theApp.SetScrollMargins(arg1, arg2); break;
      case AComm::kCueMarker:       /*c*/ ARtfHelper::sCueMarker.SetPosition(-1, arg1); break;
      case AComm::kFrameInterval:   /*f*/ AScrollDialog::gMinFrameInterval = arg1; break;
      case AComm::kContactRemote:   /*C*/ this->OnContactMessage(arg1, arg2S.c_str()); break;
   }
}

#ifdef _DEBUG
#include "utils.h"

void TRACE_LAST_ERROR(LPCSTR f, DWORD ln)
{
   DWORD err = ::GetLastError();
   CString etext = Utils::GetErrorText(err);
   CT2A ascii(etext);
   TRACE(("At %s line %d, err=%s\n"), f, ln, ascii.m_psz);
}
#else
void TRACE_LAST_ERROR(LPCSTR , DWORD ) { }
#endif

CString APubnubComm::GetLastMessage() const
{
   static CString msg;
   CA2T amsgOp(this->GetOperationName());
   CA2T amsgState(this->GetConnectionStateName());
   CA2T amsgThis(this->statusMessage.c_str());
   CA2T amsgSender(this->pSender->GetLastMessage());
   CA2T amsgReceiver(this->pReceiver->GetLastMessage());
   msg.Format(_T("%sLast operation %s completed with code %d, state:%s\nRCV:%s\nSND:%s\n"),
      (LPCTSTR)amsgThis, (LPCTSTR)amsgOp, this->statusCode, (LPCTSTR)amsgState, (LPCTSTR)amsgReceiver, (LPCTSTR)amsgSender);
   return msg;
}