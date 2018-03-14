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
   fIsMaster(false),
   fThread(NULL)
{
   fImpl = new ACommImpl(this);
   fRemote = new ACommClassImplType(this);
   this->SetState(kIdle);
}

AComm::~AComm() {
   this->Disconnect();
   delete fRemote;
   delete fImpl;
}

bool AComm::IsOnline() const {
   bool online = false;
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
   //ASSERT(username && *username && password);
   this->Disconnect();

   if (!(username && *username && password)) {
      TRACE("CANNOT CONFIGURE IP ADDRESS AT STARTUP - USE REMOTE DIALOG TO CONFIGURE LOCAL ADDRESS.\n");
      return;
   }

   this->SetState(kConnecting);
   fUsername = username;
   fPassword = password;
   fThread = ::AfxBeginThread(Connect, this);

   CT2A ascii(username); // convert from wide to narrow chars
   std::string local_addr = ascii.m_psz;
   //int port = _tstoi(password);
   //char buffer[128];
   //strncpy_s(buffer, local_addr.length(), local_addr.c_str(), 128); // debuggable buffer
   fRemote->Initialize(false, ascii.m_psz); // TEST VERSION: on startup: act as SECONDARY until told otherwise (by UI RemoteDialog button)
}

UINT AComm::Connect(LPVOID param) {
   AComm* pThis = reinterpret_cast<AComm*>(param);

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

void AComm::Disconnect() {
   if (NULL != fThread) {
      fThread->Delete();
      fThread = NULL;
   }
   if (fRemote->isSet())
      fRemote->Deinitialize();
   this->SetState(kIdle);
}

bool AComm::StartChat(LPCTSTR target) {
   // end the current one, if it exists
   this->EndChat();

   char buffer[kMaxJid] = {0}; // this is actually an IP address now for TEST version
   VERIFY(::sprintf_s(buffer, sizeof(buffer), "%S", target) < sizeof(buffer));

   if (fRemote->ChangeMode(kPrimary)) {
      // convert this end to Primary
     fIsMaster = true;
     if (fRemote->OpenLink(buffer)) {
         // SECONDARY ADDRESS FOR TESTING
         TRACE("SUCCESSFULLY CONVERTED TO PRIMARY MODE. LINK CONNECTED!\n");
         this->SetState(kChatting);
      }
      else {
         TRACE("CANNOT OPEN LINK IN PRIMARY MODE.\n");
         this->SetState(kIdle);
      }
   }
   else {
      TRACE("CANNOT CONVERT FROM SECONDARY TO PRIMARY MODE.\n");
      fIsMaster = false;
   }


   return true;
}

void AComm::EndChat() {
   if (NULL != fImpl->fChat) {
      fImpl->fChatSession->DestroyChat(fImpl->fChat);
      fImpl->fChat = NULL;
   }

   fRemote->CloseLink();
   //fIsMaster = false; // this shouldn't change, right? at least for our TEST version... one Connection request should be enough to change things around
   this->SetState(kConnected); // no longer chatting
}

bool AComm::SendFile(LPCTSTR filename) {
   ASSERT(this->IsMaster());
   if (fImpl->fFs.get() && fImpl->fChat) {
      char buffer[_MAX_PATH] = {0};
      VERIFY(::sprintf_s(buffer, _countof(buffer), "%S", filename) < mCountOf(buffer));
      fImpl->fFs->SendFile(buffer, fImpl->fChat->remote_jid());
   }
   return true;
}

bool AComm::SendCommand(Command cmd, int param1, int param2) {
   char buffer[32] = {0};
   VERIFY(::sprintf_s(buffer, "%d,%d", param1, param2) < mCountOf(buffer));
   return this->SendCommand(cmd, buffer);
}

bool AComm::SendCommand(Command cmd, const std::string& param) {
   TRACE("R>%c%s\n", cmd, param.c_str()); // send these to fRemote
   //if (NULL != fImpl->fChatSession) {
      char buffer[32] = {0};
      VERIFY(::sprintf_s(buffer, _countof(buffer), "%c%s", cmd, param.c_str()) < mCountOf(buffer));
      //fImpl->fChatSession->SendChatMessage("", buffer, XmppChat::CHAT_STATE_ACTIVE);
      fRemote->SendCommand(buffer);
      return true;
   //}
   return false;
}

void AComm::SetParent(HWND parent) {
   fImpl->SetParent(parent);
   fRemote->SetParent(parent);
}

CString AComm::GetSessionJid() const {
   return fImpl->fChat ? fImpl->fChat->remote_jid().Str().c_str() : CString();
}

bool AComm::Read() {
   fUsername = theApp.GetProfileString(gStrComm, gStrCommUsername, _T("username@gmail.com"));
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
