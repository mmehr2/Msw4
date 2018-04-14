#include "stdafx.h"
#include "Comm.h"
#include "resource.h"
#include "Msw.h"
#include "strings.h"
#include "RtfHelper.h"
#include "ScrollDialog.h"

#include <Iphlpapi.h>
#pragma comment (lib, "Iphlpapi")

//-LibJingle ------------------------------------------------------------------
#pragma warning (push)
#pragma warning (disable: 4310 4996)
#include <iomanip>
#include "talk/base/helpers.h"
#include "talk/base/win32.h"
#include "talk/base/logging.h"
#include "talk/base/physicalsocketserver.h"
#include "talk/base/stringencode.h"

#include "talk/examples/login/presenceouttask.h"
#include "talk/examples/login/xmppthread.h"

#include "talk/xmpp/constants.h"
#include "talk/xmpp/jid.h"
#include "talk/xmllite/xmlelement.h"
#include "talk/session/chat/xmpp_chat.h"
#include "talk/session/chat/xmpp_chat_session_client.h"

// File transfer
#include <direct.h>
#include "talk/base/pathutils.h"
#include "talk/base/fileutils.h"
#include "talk/p2p/client/httpportallocator.h"
#include "talk/p2p/client/sessionmanagertask.h"
#include "talk/session/fileshare/fileshare.h"
#include "talk/examples/login/jingleinfotask.h"
#pragma warning (pop)

#define REM_PUB

// MLM: Pubnub
#ifdef REM_PUB
#include "PubnubComm.h"
static APubnubComm* fRemote = NULL; // don't use it directly, we can change the implementation class later; try to keep the API pure, devoid of Pubnub-isms
typedef APubnubComm ACommClassImplType;
#endif

// MLM: Custom
#ifdef REM_CUSTOM
#include "CustomComm.h"
static ACustomComm* fRemote = NULL;
typedef ACustomComm ACommClassImplType;
#endif

//-----------------------------------------------------------------------------
// settings
static const int kRecvTimeout          = 1;              // seconds
static const int kConnectPeriod        = 1000;           // milliseconds between connect retries
static const int kMaxJid               = 256;
static const char* talk_server         = "192.168.1.136"; //"talk.google.com"; // MLM-local testing with ejabberd on this machine
static const int talk_server_port      = 5222;

class FileShareClient : public sigslot::has_slots<> {
public:
   FileShareClient(buzz::XmppClient *xmppclient);

   bool SendFile(const std::string& filename, const buzz::Jid& jid);
   void OnFileShareSessionCreate(cricket::FileShareSession *sess);
   void OnJingleInfo(const std::string & relay_token, 
      const std::vector<std::string> &relay_addresses, 
      const std::vector<talk_base::SocketAddress> &stun_addresses);

private:
   void OnSessionState(cricket::FileShareState state);

public:
   talk_base::NetworkManager network_manager_;
   talk_base::scoped_ptr<cricket::HttpPortAllocator> port_allocator_;
   talk_base::scoped_ptr<cricket::SessionManager> session_manager_;
   talk_base::scoped_ptr<cricket::FileShareSessionClient> file_share_session_client_;
   buzz::XmppClient* fClient;
   buzz::Jid fSendTo;
   cricket::FileShareSession *session_;
   bool waiting_for_file_;
};

class ACommImpl : public sigslot::has_slots<> {
public:
   ACommImpl(AComm* comm);

   void OnStateChange(buzz::XmppEngine::State state);

   void SetParent(HWND parent);

   buzz::Jid fJid;
   buzz::XmppClient* fClient;
   XmppChat* fChat;
   XmppChatSessionClient* fChatSession;
   std::auto_ptr<FileShareClient> fFs;

private:
   AComm* fComm;
   HWND fParent;
};

class DebugLog : public sigslot::has_slots<> {
public:
   DebugLog();

   char* debug_input_buf_;
   int debug_input_len_;
   int debug_input_alloc_;
   char* debug_output_buf_;
   int debug_output_len_;
   int debug_output_alloc_;
   bool censor_password_;

   void Input(const char * data, int len);
   void Output(const char * data, int len);
   static bool IsAuthTag(const char * str, size_t len);
   void DebugPrint(char * buf, int * plen, bool output);
};
static DebugLog debug_log_;

AComm::AComm() :
   fState(kUnknown),
#ifdef PRIMARY
   fIsMaster(true),
#else
   fIsMaster(false),
#endif //def PRIMARY
   fThread(NULL)
{

   fImpl = new ACommImpl(this);
   fRemote = new ACommClassImplType(this);
   this->SetState(kIdle);
}

bool AComm::ConfigureSettings() 
{
   // NOTE: is the call to theApp.SetRegistryKey() made already? are we relying on order of static initialization here? try not to..
   // Maybe the comm object can be heap allocated by theApp.InitInstance()? otherwise we must set this flag from there
   fIsMaster = TRUE == theApp.GetProfileInt(gStrComm, gStrCommPrimary, FALSE);
   TRACE(_T("Derived PRIMARY flag %d from Registry Key %s\n"), fIsMaster, theApp.m_pszRegistryKey);
   fRemote->Configure();
   return true;
}

AComm::~AComm() {
   this->Disconnect();
   delete fRemote;
   delete fImpl;
}

bool AComm::IsOnline() const {
   bool online = false;
   // MLM - not sure what this is doing; and does it work if there is a proxy server?
   DWORD dwTableSize = 0;
   ::GetIpForwardTable(NULL, &dwTableSize, FALSE);
   std::auto_ptr<BYTE> buffer = std::auto_ptr<BYTE>(new BYTE[dwTableSize]);
   MIB_IPFORWARDTABLE* forwardTable = (MIB_IPFORWARDTABLE*)buffer.get();
   if (NO_ERROR == ::GetIpForwardTable(forwardTable, &dwTableSize, TRUE))
   {
      for (UINT i = 0; !online && (i < forwardTable->dwNumEntries); ++i)
         if (0 == forwardTable->table[i].dwForwardDest)
            online = true;
   }

   return online;
} 

void AComm::Connect(LPCTSTR username, LPCTSTR password) {

   if (fState >= kConnected) {
      //this->Disconnect(); // this is asynchronous...
      // already done
      this->OnStateChange(kLogin, kSuccess);
      return;
   }

   //ASSERT(username && *username && password);
   if (!(username && *username && password)) {
      TRACE("CANNOT CONFIGURE IP ADDRESS AT STARTUP - USE REMOTE DIALOG TO CONFIGURE LOCAL ADDRESS.\n");
      return;
   }

   fUsername = username;
   fPassword = password;
   //fThread = ::AfxBeginThread(Connect, this);

   CT2A ascii(username); // convert from wide to narrow chars
   //std::string local_addr = ascii.m_psz;
   //int port = _tstoi(password);
   //char buffer[128];
   //strncpy_s(buffer, local_addr.length(), local_addr.c_str(), 128); // debuggable buffer
   this->SetState(kConnecting);
   fRemote->Login(ascii.m_psz);
   this->RemoteBusyWait();
   if (fRemote->isSuccessful())
      this->SetState(kConnected);
   else
      this->SetState(kIdle); // with failure code reported
}

UINT AComm::Connect(LPVOID /*param*/) {
   //AComm* pThis = reinterpret_cast<AComm*>(param);

   // NOTE: This thread is reserved for proxy negotiation on startup (i.e. startup of the program).
   // The call in pubnub requires much time. Do it here using private context object.
   // Once done, this needs to be saved and a done flag set (with critical section).
   // Then, when Login is finally called, it must fail with visible error if the proxy info is not ready.
   // If it's ready, it can proceed to transfer the info to the Sender and Receiver contexts (SDK 2.3.2 cannot do this tho!).
   // It still not, perhaps we need to trigger the individual contexts to each negotiate their own here (twice).
   // This can still be done on startup for the two contexts instead of the private one, it just takes 2X longer.
   // We still need to have the critical section and flag, however.

   /* 2.3.2 Design:
   > CRITICAL SECTION
   > proxyNegotiations done flag (overall)
   > result indicator (success/failure + error result code and/or message)
   > NegotiateProxy() function for both SendChannel and ReceiveChannel (operates the same tho) - where to put it? - Common Base Class of course...
   . Use wait call (sync?) to tell when done, since on private thread.
   .. If successful with first call, start second.
   .. If successful with second call, set done flag as successful.
   .. If either one has errors, record errors and allow Login to check to get error response.
   . On Login, fail without calling Init() if proxy is required and failed, or not ready yet. Report proper UI results.
   .. If success and done, the proxy info is in place, so no problem.
   . (Perhaps each context can check itself? So then each channel knows how to fail its Init() call if it has no proxy info when needed.)
   */

   //fRemote->PublishQueueThreadFunction(fRemote);
   //char buffer[kMaxJid] = {0};
   //VERIFY(sprintf_s(buffer, sizeof(buffer), "%S", pThis->fUsername) < sizeof(buffer));
   //pThis->fImpl->fJid = buzz::Jid(buffer);
   //buzz::XmppClientSettings xcs;
   //xcs.set_user(pThis->fImpl->fJid.node());
   //xcs.set_resource("MSW");
   //xcs.set_host(pThis->fImpl->fJid.domain());
   //xcs.set_use_tls(false); // MLM - no TLS for now

   //VERIFY(sprintf_s(buffer, sizeof(buffer), "%S", pThis->fPassword) < sizeof(buffer));
   //talk_base::InsecureCryptStringImpl pass;
   //pass.password() = buffer;
   //xcs.set_pass(talk_base::CryptString(pass));
   //xcs.set_server(talk_base::SocketAddress(talk_server, talk_server_port));
   //TRACE("MLM: Connection to talk server will be through %s:%d\n", talk_server, talk_server_port);

   //talk_base::PhysicalSocketServer ss;
   //talk_base::Thread main_thread(&ss);
   //talk_base::ThreadManager::SetCurrent(&main_thread);

   //XmppPump pump;
   //pThis->fImpl->fClient = pump.client();
   //pump.client()->SignalLogInput.connect(&debug_log_, &DebugLog::Input);
   //pump.client()->SignalLogOutput.connect(&debug_log_, &DebugLog::Output);
   //pump.client()->SignalStateChange.connect(pThis->fImpl, &ACommImpl::OnStateChange);

   //pump.DoLogin(xcs, new XmppSocket(false), NULL);
   //main_thread.Run();
   //pump.DoDisconnect();

   return 0;
}

void AComm::OnStateChange(OpType operation, Status statusCode) {
   // **NOTE**: this will be called on a non-UI thread in most cases 
   // for now, we will NOT make any local state changes here (CRITICAL SECTION NEEDED FOR THAT)
   // each function can examine the status code that caused this and change state accordingly
   // post the status message to the UI (no CS for that)
   fRemote->SendMsg( WMA_UPDATE_STATUS, (WPARAM)operation, (LPARAM)statusCode );
}

void AComm::ResendLastStatus() const
{
   fRemote->SendStatusReport();
}

void AComm::RemoteBusyWait() {
   //while (fRemote->isBusy()) {
   //   Sleep(10);
   //}
   do {  // check for messages while we're killing time...
      MSG msg;
      if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {  // we've got messages in our queue
         ::TranslateMessage(&msg);
         ::DispatchMessage(&msg);
      }
   } while (fRemote->isBusy());
}

void AComm::Disconnect() {
   if (NULL != fThread) {
      fThread->Delete();
      fThread = NULL;
   }

   // end any conversation, if it exists
   if (fState == kChatting) {
      this->EndChat(); // but need to ignore the UI sub-response
      // and if failed to unpair, we must fail the disconnect, as well
      if (fState == kChatting) {
         this->OnStateChange(kLogout, kUnableToUnpair);
         return;
      }
   }

   this->SetState(kDisconnecting);
   fRemote->Logout(); // but this is asynchronous ...

   this->RemoteBusyWait();

   if (fRemote->isSuccessful())
      this->SetState(kIdle);
   else
      this->SetState(kConnected); // with failure code reported
}

//#define USE_CONTACT
//#define USE_CONTACT2

bool AComm::IsContacted() const
{
#ifdef USE_CONTACT2
   int ccode = fRemote->GetContactCode();
   return ccode == kSuccess;
#else
   return true;
#endif
}

bool AComm::StartChat(LPCTSTR target) {
   ASSERT(this->IsMaster());

   char buffer[kMaxJid] = {0}; // this is device name of secondary
   VERIFY(::sprintf_s(buffer, sizeof(buffer), "%S", target) < sizeof(buffer));

   // verify that we are talking to this device or not; shut down conversation with other device if needed first
   if (fState == kChatting) {
      if (fRemote->GetConnectedName() != target) {
         // end the current chat and continue to establish a new one
         this->EndChat();
#ifdef USE_CONTACT
      } else if (this->IsContacted()) {
         // we are chatting with the right device, no need to kill it
         // but we need to check if we completed the Contact command OK
         // if we have, we need to trigger the completion transaction
         this->OnStateChange(kContact, (Status)fRemote->GetStatusCode());
         return true;
#endif // USE_CONTACT
      } else {
         // else we can skip the OpenLink and go directly to the SendCommandBusy(kContact) call
         goto JustContact;
      }
   } else if (fState != kConnected) {
      // not able to start conversation if not online properly
      // BUT SHOULD WE TRY ANYWAY??
      // add status update line too
      this->OnStateChange(kConnect, (Status)fRemote->GetStatusCode());
      return false;
   }

   // Step 1 - open the channel link
   fRemote->OpenLink(buffer);

   this->RemoteBusyWait();
   bool result = fRemote->isSuccessful();

   if (result) {
      // We have a connection with the Secondary, but the Contact command might have had issues
      this->SetState(kChatting);
   }
   else
      this->SetState(kConnected); // with failure code reported

JustContact:
#ifdef USE_CONTACT
   // NEEDS DEBUGGING - make it a separate TEST command button for now
   // Step 2 - send the contact command
   fRemote->SendCommandBusy(kContact);

   this->RemoteBusyWait();
   result = fRemote->isSuccessful();
   int ccode = fRemote->GetContactCode();

   CString msg, msg2 = fRemote->GetConnectedName();
   if (!result) {
      // button has to allow Connect again
      msg.Format(_T("Unable to Connect to SECONDARY %s\n"), msg2);
   } else if (ccode == kContactRejected) {
      // button has to allow Connect again
      // display status line
      msg.Format(_T("SECONDARY %s has rejected connection request (busy).\n"), msg2);
   } else if (ccode == kSuccess) {
      // button can say Disconnect now
      // display status line
      msg.Format(_T("Connected to SECONDARY %s in %1.6lf sec\n"), msg2, fRemote->GetCommandTimeSecs());
   } else {
      // button has to allow Connect again
      msg.Format(_T("Bad contact reply from SECONDARY %s\n"), msg2);
   }
   ::AfxMessageBox(msg);
#endif // USE_CONTACT

   return result;
}

void AComm::EndChat() {
   bool result;

#ifdef USE_CONTACT
   if (this->IsContacted()) {
      // tell the paired secondary we are going away (release it for others)
      fRemote->SendCommandBusy(kContactCancel);
      // wait until published, proceed whether or not it was successfully sent
      this->RemoteBusyWait();
      result = fRemote->isSuccessful();
      if (result)
         TRACE("SUCCESSFULLY SENT UNPAIR COMMAND TO SECONDARY REMOTE.\n");
      else
         TRACE("UNABLE TO SEND UNPAIR COMMAND TO SECONDARY REMOTE.\n");
   }
#endif // USE_CONTACT

   // always do remote functions, to provide UI responses when done
   fRemote->CloseLink();

   this->RemoteBusyWait();
   result = fRemote->isSuccessful();

   if (result)
      this->SetState(kConnected); // no longer chatting
   else
      // might need to check actual error scenario (code) to determine best state here
      this->SetState(kChatting); // with failure code reported
}

bool AComm::SendFile(LPCTSTR /*filename*/) {
   ASSERT(this->IsMaster());
   //if (fImpl->fFs.get() && fImpl->fChat) {
   //   char buffer[_MAX_PATH] = {0};
   //   VERIFY(::sprintf_s(buffer, _countof(buffer), "%S", filename) < mCountOf(buffer));
   //   fImpl->fFs->SendFile(buffer, fImpl->fChat->remote_jid());
   //}

   // PRIMARY -- NOTE: This is for all the various things that need to precede an actual Pubnub scrolling session:
   // Step 1 - send Preferences message (video settings, etc.) from dialog box
   fRemote->SendOperation(kPrefsSend);
   // Step 2 - turn on scrolling mode (2-way back-channel reception)
   fRemote->StartScrollMode();
   // Step 3 - send the script/file transfer start message (other messages triggered by Secondary as needed to get data)
   // Step 4 - wait for Secondary xfer done response before returning (which starts scrolling)
   return true;
}

bool AComm::SendCommand(Command cmd, int param1, int param2) {
   // special handling for scroll shutoff command (just before last command is sent)
   if (cmd == AComm::kScroll && param1 == AComm::kOff) {
      fRemote->StopScrollMode();
   }

   char buffer[32] = {0};
   VERIFY(::sprintf_s(buffer, "%d,%d", param1, param2) < mCountOf(buffer));
   return this->SendCommand(cmd, buffer);
}

bool AComm::SendCommand(Command cmd, const std::string& param) {
   TRACE("R>%c%s\n", cmd, param.c_str()); // send these to fRemote
   if (fRemote->isConversing()) {
      char buffer[32] = {0};
      VERIFY(::sprintf_s(buffer, _countof(buffer), "%c%s", cmd, param.c_str()) < mCountOf(buffer));
      //fImpl->fChatSession->SendChatMessage("", buffer, XmppChat::CHAT_STATE_ACTIVE);
      fRemote->SendCommand(buffer);
      return true;
   }
   return false;
}

void AComm::RunTest(LPCTSTR params_) 
{
   CT2A params_X(params_);
   CStringA params = params_X.m_psz;
   fRemote->OnMessage(params_X);
}

void AComm::SetParent(HWND parent) {
   fImpl->SetParent(parent);
   fRemote->SetParent(parent);
}

HWND AComm::GetParent(void) const {
   return fRemote->GetParent();
}

CString AComm::GetLastMessage(void) const {
   return fRemote->GetLastMessage();
}

CString AComm::GetSessionJid() const {
   return fRemote->isConversing() ? fRemote->GetConnectedName() : CString();
}

bool AComm::Read() {
   fUsername = theApp.GetProfileString(gStrComm, gStrCommUsername, _T(""));
   fPassword = theApp.GetProfileString(gStrComm, gStrCommPassword);

   fSlaves.clear();
   for (DWORD i = 0; i < UINT_MAX; ++i)
   {
      CString entry;
      entry.Format(_T("%s%d"), gStrCommSlave, i);
      ASlave slave;
      slave.fUsername = theApp.GetProfileString(gStrComm, entry);
      if (!slave.fUsername.IsEmpty())
         fSlaves.push_back(slave);
      else
         break;
   }

   return true;
}

bool AComm::Write() {
   bool result = true;

   theApp.WriteProfileString(gStrComm, gStrCommUsername, fUsername);
   theApp.WriteProfileString(gStrComm, gStrCommPassword, fPassword);

   for (size_t i = 0; i < UINT_MAX; ++i)
   {
      CString entry;
      entry.Format(_T("%s%d"), gStrCommSlave, i);
      if (i < fSlaves.size())
      {  // write this one
         if (!theApp.WriteProfileString(gStrComm, entry, fSlaves[i].fUsername))
            result = false;
      }
      else if (!theApp.GetProfileString(gStrComm, entry).IsEmpty())
      {  // empty this one
         theApp.WriteProfileString(gStrComm, entry, _T(""));
      }
      else
         break;   // done
   }
   return result;
}

ACommImpl::ACommImpl(AComm* comm) : 
   fComm(comm), 
   fClient(NULL),
   fChat(NULL),
   fChatSession(NULL)
{
   ASSERT(NULL != fComm); 
}

void ACommImpl::SetParent(HWND parent) {
   fParent = parent;
   if (NULL != fChatSession) {
      fChatSession->SetParent(parent);
   }
}

void ACommImpl::OnStateChange(buzz::XmppEngine::State state) {
   switch (state) {
      case buzz::XmppEngine::STATE_START:    TRACE(("Connecting...\n")); fComm->SetState(AComm::kConnecting); break;
      case buzz::XmppEngine::STATE_OPENING:  TRACE(("Logging in.\n")); fComm->SetState(AComm::kConnecting); break;
      case buzz::XmppEngine::STATE_CLOSED:   TRACE(("Logged out.\n")); fComm->SetState(AComm::kIdle); break;
      
      case buzz::XmppEngine::STATE_OPEN:
      {
         TRACE("Logged in as %S\n", fJid.Str().c_str());
         fComm->SetState(AComm::kConnected);

         std::string client_unique = fClient->jid().Str();
         cricket::InitRandom(client_unique.c_str(), client_unique.size());

         // broadcast our presence
         buzz::Status my_status;
         my_status.set_jid(fClient->jid());
         my_status.set_available(true);
         my_status.set_show(buzz::Status::SHOW_ONLINE);
         my_status.set_priority(0);
         my_status.set_know_capabilities(true);
         my_status.set_fileshare_capability(true);
         my_status.set_is_google_client(true);
         my_status.set_version("1.0.0.66");

         buzz::PresenceOutTask* presence = new buzz::PresenceOutTask(fClient);
         presence->Send(my_status);
         presence->Start();

//fFs.reset(new FileShareClient(fClient));
//fFs->port_allocator_.reset(new cricket::HttpPortAllocator(&fFs->network_manager_, "MSW"));
//fFs->session_manager_.reset(new cricket::SessionManager(fFs->port_allocator_.get(), NULL));
//
//cricket::SessionManagerTask* session_manager_task = new cricket::SessionManagerTask(fClient, fFs->session_manager_.get());
//session_manager_task->EnableOutgoingMessages();
//session_manager_task->Start();
//
//buzz::JingleInfoTask *jingle_info_task = new buzz::JingleInfoTask(fClient);
//jingle_info_task->RefreshJingleInfoNow();
//jingle_info_task->SignalJingleInfo.connect(fFs.get(), &FileShareClient::OnJingleInfo);
//jingle_info_task->Start();
//
//fFs->file_share_session_client_.reset(new cricket::FileShareSessionClient(fFs->session_manager_.get(), fClient->jid(), "MSW"));
//fFs->file_share_session_client_->SignalFileShareSessionCreate.connect(fFs.get(), &FileShareClient::OnFileShareSessionCreate);
//fFs->session_manager_->AddClient(NS_GOOGLE_SHARE, fFs->file_share_session_client_.get());

         // begin a chat session
         fChatSession = new XmppChatSessionClient(fClient, fParent);
         fChatSession->Start();
      }
      break;
   }
}


//-----------------------------------------------------------------------------
// File transfer
FileShareClient::FileShareClient(buzz::XmppClient *xmppclient) :
   fClient(xmppclient)
{
}

bool FileShareClient::SendFile(const std::string& filename, const buzz::Jid& jid) {
   cricket::FileShareManifest* manifest = new cricket::FileShareManifest();
   size_t size = 0;
   talk_base::Filesystem::GetFileSize(filename, &size);
   manifest->AddFile(filename, size);

   cricket::FileShareSession* share = file_share_session_client_->CreateFileShareSession();
   share->Share(jid, manifest);
   return true;
}

void FileShareClient::OnJingleInfo(const std::string & relay_token, 
   const std::vector<std::string> &relay_addresses, 
   const std::vector<talk_base::SocketAddress> &stun_addresses) {
   port_allocator_->SetStunHosts(stun_addresses);
   port_allocator_->SetRelayHosts(relay_addresses);
   port_allocator_->SetRelayToken(relay_token);
}

void FileShareClient::OnSessionState(cricket::FileShareState state) {
   switch(state) {
      case cricket::FS_OFFER:
      // The offer has been made; print a summary of it and, if it's an incoming transfer, accept it
      TRACE("%S file %S\n", session_->is_sender() ? "Offering" : "Receiving", 
         session_->manifest()->item(0).name.c_str());
      if (!session_->is_sender() && waiting_for_file_) {
         session_->Accept();
         waiting_for_file_ = false;
      }
      break;

      case cricket::FS_TRANSFER:       TRACE("File transfer started\n"); break;
      case cricket::FS_COMPLETE:       TRACE("\nFile transfer completed\n"); break;
      case cricket::FS_LOCAL_CANCEL:
      case cricket::FS_REMOTE_CANCEL:  TRACE("\nFile transfer cancelled\n"); break;
      case cricket::FS_FAILURE:        TRACE("\nFile transfer failed\n"); break;
   }
}

void FileShareClient::OnFileShareSessionCreate(cricket::FileShareSession *sess) {
   char cwd[_MAX_PATH] = {0};
   ::_getcwd(cwd, sizeof(cwd));
   session_ = sess;
   sess->SignalState.connect(this, &FileShareClient::OnSessionState);
   sess->SetLocalFolder(cwd);
}


//-----------------------------------------------------------------------------
// Debugging
DebugLog::DebugLog() : debug_input_buf_(NULL), debug_input_len_(0), 
   debug_input_alloc_(0), debug_output_buf_(NULL), debug_output_len_(0), 
   debug_output_alloc_(0), censor_password_(false)
{
}

void DebugLog::Input(const char * data, int len) {
   if (debug_input_len_ + len > debug_input_alloc_) {
      char * old_buf = debug_input_buf_;
      debug_input_alloc_ = 4096;
      while (debug_input_alloc_ < debug_input_len_ + len) {
         debug_input_alloc_ *= 2;
      }
      debug_input_buf_ = new char[debug_input_alloc_];
      memcpy(debug_input_buf_, old_buf, debug_input_len_);
      delete[] old_buf;
   }
   memcpy(debug_input_buf_ + debug_input_len_, data, len);
   debug_input_len_ += len;
   DebugPrint(debug_input_buf_, &debug_input_len_, false);
}

void DebugLog::Output(const char * data, int len) {
   if (debug_output_len_ + len > debug_output_alloc_) {
      char * old_buf = debug_output_buf_;
      debug_output_alloc_ = 4096;
      while (debug_output_alloc_ < debug_output_len_ + len) {
         debug_output_alloc_ *= 2;
      }
      debug_output_buf_ = new char[debug_output_alloc_];
      memcpy(debug_output_buf_, old_buf, debug_output_len_);
      delete[] old_buf;
   }
   memcpy(debug_output_buf_ + debug_output_len_, data, len);
   debug_output_len_ += len;
   DebugPrint(debug_output_buf_, &debug_output_len_, true);
}

bool DebugLog::IsAuthTag(const char * str, size_t len) {
   if (str[0] == '<' && str[1] == 'a' && str[2] == 'u' &&
      str[3] == 't' && str[4] == 'h' && str[5] <= ' ') {
      std::string tag(str, len);

      if (tag.find("mechanism") != std::string::npos)
         return true;
   }
   return false;
}

void DebugLog::DebugPrint(char * buf, int * plen, bool output) {
   int len = *plen;
   if (len > 0) {
      time_t tim = time(NULL);
      struct tm now;
      localtime_s(&now, &tim);
      char time_string[64] = {0};
      asctime_s(time_string, sizeof(time_string), &now);
      if (time_string) {
         size_t time_len = strlen(time_string);
         if (time_len > 0) {
            time_string[time_len-1] = 0;    // trim off terminating \n
         }
      }
      LOG(INFO) << (output ? "SEND >>>" : "RECV <<<") << " : " << time_string;
      bool indent;
      int start = 0, nest = 3;
      for (int i = 0; i < len; i += 1) {
         if (buf[i] == '>') {
            if ((i > 0) && (buf[i-1] == '/')) {
               indent = false;
            } else if ((start + 1 < len) && (buf[start + 1] == '/')) {
               indent = false;
               nest -= 2;
            } else {
               indent = true;
            }

            // Output a tag
            LOG(INFO) << std::setw(nest) << " " << std::string(buf + start, i + 1 - start);

            if (indent)
               nest += 2;

            // Note if it's a PLAIN auth tag
            if (IsAuthTag(buf + start, i + 1 - start)) {
               censor_password_ = true;
            }

            // incr
            start = i + 1;
         }

         if (buf[i] == '<' && start < i) {
            if (censor_password_) {
               LOG(INFO) << std::setw(nest) << " " << "## TEXT REMOVED ##";
               censor_password_ = false;
            } else {
               LOG(INFO) << std::setw(nest) << " " << std::string(buf + start, i - start);
            }
            start = i;
         }
      }
      len = len - start;
      memcpy(buf, buf + start, len);
      *plen = len;
   }
}
