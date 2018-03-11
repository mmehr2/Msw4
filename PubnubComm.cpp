#include "stdafx.h"
#include "PubnubComm.h"

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

static const std::string channel_separator = "-"; // avoid Channel name disallowed characters
static const std::string app_name = "MSW"; // prefix for all channel names by convention

//extern "C" {
   void pn_callback(pubnub_t* pn, pubnub_trans t, pubnub_res res, void* pData);
//}

class PNChannelInfo {
public:
   std::string key;
   std::string key2;
   std::string channelName;
   bool is_remote;
   pubnub_t * pContext;
   //void * pConnection; //?? what waits for data??
   std::string op_msg;

   PNChannelInfo(/*pubnub_t * pC*/);
   ~PNChannelInfo();

   bool Init(bool is_primary);
   bool DeInit();

   bool Send(const std::string& message);

   const char* GetTypeName();
private:
   PNChannelInfo(const PNChannelInfo& other);
   PNChannelInfo& operator=(const PNChannelInfo& other);
};

PNChannelInfo::PNChannelInfo(/*pubnub_t * pC*/) 
   : key("demo")
   , key2("demo")
   , channelName("")
   , is_remote(false)
   , pContext(nullptr)
   //, pConnection(nullptr)
{
}

const char* PNChannelInfo::GetTypeName()
{
   if (this->is_remote)
      return "REMOTE";
   else
      return "LOCAL";
}

bool PNChannelInfo::Init(bool is_publisher)
{
   pubnub_res res;
   if (pContext)
      pubnub_free(pContext);
   pContext = pubnub_alloc();
   is_remote = is_publisher;
   if (is_remote)
      pubnub_init(pContext, key.c_str(), key2.c_str()); // remote publishes on this channel only
   else
      pubnub_init(pContext, "", key.c_str()); // local subscribes on this channel only
   res = pubnub_register_callback(pContext, &pn_callback, (void*) this);
   if (res != PNR_OK)
      return false; // error reporting needed!
   // start to listen on this channel if local
   if (!is_remote)
   {
      res = pubnub_subscribe(pContext, this->channelName.c_str(), NULL);
      if (res != PNR_STARTED)
         return false; // error reporting needed!
   }
   return true;
}

bool PNChannelInfo::Send(const std::string& message)
{
   pubnub_res res;
   op_msg = "";
   if (pContext == nullptr)
      return false;
   res = pubnub_publish(pContext, this->channelName.c_str(), message.c_str());
   if (res != PNR_STARTED) {
      op_msg += pubnub_last_publish_result(pContext);
      return false; // error reporting needed!
   }
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
   // NOTE: by using PrivateProfile, I can guarantee a file will be found (in the Windows directory, since I don't specify the full path)
   // Otherwise, Windows prefers that you use the registry, which is much harder for the customer to edit when configuring their own accounts.
   char buffer[1024];
   DWORD res;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "Pubkey", "demo", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: Pubkey returned %d: %s\n", res, buffer);
   this->pRemote->key = buffer;
   res = ::GetPrivateProfileStringA("MSW_Pubnub", "Subkey", "demo", buffer, sizeof(buffer), "msw.ini");
   TRACE("CONFIG: Subkey returned %d: %s\n", res, buffer);
   this->pLocal->key = buffer;
   // fix for publish "Invalid subscribe key" error
   this->pRemote->key2 = this->pLocal->key;
   // load the company/customer and device IDs
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

   // TBD: publish message to pRemote->channelName and set up callback
   // PRIMARY: will send commands
   // SECONDARY: will send any responses
   std::string msg = JSONify(message);
   TRACE("SENDING MESSAGE %s:", msg.c_str()); // DEBUGGING
   if (false == pRemote->Send(msg)) {
      TRACE(pRemote->op_msg.c_str());
   }
   TRACE("\n");
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

const char* GetTransactionName(pubnub_trans t)
{
   const char* result = "UNSUPPORTED";
   switch (t) {
   case PBTT_NONE:
      result = "NO-OP";
      break;
   case PBTT_SUBSCRIBE:
      result = "SUBSCRIBE";
      break;
   case PBTT_PUBLISH:
      result = "PUBLISH";
      break;
   default:
      break;
   }
   return result;
}

const char* GetResultName(pubnub_res res)
{
   const char* result = "UNSUPPORTED";
   switch (res) {
   case PNR_OK:
      result = "OK";
      break;
   case PNR_CONNECT_FAILED:
      result = "NO-SERVER";
      break;
   case PNR_CONNECTION_TIMEOUT:
      result = "NET-TIMEOUT";
      break;
   case PNR_TIMEOUT:
      result = "OP-TIMEOUT";
      break;
   case PNR_ABORTED:
      result = "ABORTED";
      break;
   case PNR_IO_ERROR:
      result = "IO-ERROR";
      break;
   case PNR_HTTP_ERROR:
      result = "HTTP-ERROR"; // call pubnub_last_http_code()
      break;
   case PNR_FORMAT_ERROR:
      result = "BAD-JSON-RCV";
      break;
   case PNR_CANCELLED:
      result = "CANCEL";
      break;
   case PNR_STARTED:
      result = "PLS-WAIT"; // normal
      break;
   case PNR_IN_PROGRESS:
      result = "BUSY";
      break;
   case PNR_RX_BUFF_NOT_EMPTY:
      result = "RX-NOTEMPTY";
      break;
   case PNR_TX_BUFF_TOO_SMALL:
      result = "TX-SMALL";
      break;
   case PNR_INVALID_CHANNEL:
      result = "BAD-CHNAME";
      break;
   case PNR_PUBLISH_FAILED:
      result = "BAD-PUB"; // need to see pubnub_last_publish_result()
      break;
   case PNR_CHANNEL_REGISTRY_ERROR:
      result = "CHANREG";
      break;
   case PNR_REPLY_TOO_BIG:
      result = "REPLY-BUF";
      break;
   case PNR_INTERNAL_ERROR:
      result = "INTERNAL";
      break;
   case PNR_CRYPTO_NOT_SUPPORTED:
      result = "CRYPTO";
      break;
   default:
      break;
   }
   return result;
}

// function to capture pubnub_log output to the TRACE() window (ONLY FOR OUR CODE)
void pn_printf(char* fmt, ...)
{
   char buffer[10240];

    va_list args;
    va_start(args,fmt);
    vsprintf(buffer,fmt,args);
    va_end(args);

    TRACE("%s\n", buffer);
}

// helper to return the global MFC thread ID, if the caller is running on that thread
DWORD GetThreadIDEx()
{
   CWinThread* pThread = ::AfxGetThread();
   DWORD thrID = 0;
   if (pThread)
      thrID = pThread->m_nThreadID;
   return thrID;
}

void pn_callback(pubnub_t* pn, pubnub_trans t, pubnub_res res, void* pData)
{
   // General debugging on the callback for now
   PNChannelInfo* pChannel = static_cast<PNChannelInfo*>(pData);
   const char* cname = pChannel ? (pChannel->channelName.c_str()) : "NONE";
   std::string op;
   if (res != PNR_OK && t == PBTT_PUBLISH && pChannel) {
      op = pubnub_last_publish_result(pChannel->pContext);
      pChannel->op_msg += op;
      op = "\t..." + op; // has its own endl
   }
   TRACE("@*@_CB> %s IN %s ON: %s (T=%X)\n%s", 
      GetResultName(res), GetTransactionName(t), cname, GetThreadIDEx(), op.c_str());
   pChannel = nullptr; // DEBUG BREAKS CAN GO HERE
}

bool APubnubComm::ConnectHelper(PNChannelInfo* pChannel)
{
   bool result = false;

   TRACE(_T("CONNHELP: Current Thread ID = 0x%X\n"), GetThreadIDEx());

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
   TRACE(_T("DISCONNHELP: Current Thread ID = 0x%X, CTX=%X\n"), GetThreadIDEx(), pCtx);

   if (pChannel->DeInit())
   {
      TRACE("CH> DELETED %s CHANNEL %X\n", pChannel->GetTypeName(), pCtx);
      result = true;
   }

   return result;
}

// Essential JSON from https://github.com/petehug/wjelement-cpp/blob/master/src/wjelement%2B%2B.cpp
#pragma warning(push)
#pragma warning(disable: 4245 4018 4996 )

	size_t wjWUTF8CharSize(char *str, size_t length)
	{
		size_t			r	= -1;
		unsigned char	test;
		int				i;

		if (!str || !length) {
			return(0);
		}

		test = (unsigned char) *str;

		if (!(test & 0x80)) {
			/* ASCII */
			return(1);
		} else if ((test & 0xC0) == 0x80) {
			/*
				This is not a primary octet of a UTF8 char, but is valid as any
				other octet of the UTf8 char.
			*/
			return(-1);
		} else if ((test & 0xE0) == 0xC0) {
			if (test == 0xC0 || test == 0xC1) {
				/* redundant code point. */
				return(-1);
			}
			r = 2;
		} else if ((test & 0xF0) == 0xE0) {
			r = 3;
		} else if ((test & 0xF8) == 0xF0) {
			if ((test & 0x07) >= 0x05) {
				/* undefined character range. RFC 3629 */
				return(-1);
			}

			r = 4;
		} else {
			/*
				Originally there was room for (more 4,) 5 and 6 byte characters but
				these where outlawed by RFC 3629.
			*/
			return(-1);
		}

		if (r > length) {
			r = length;
		}

		for (i = 1; i < r; i++) {
			if (((unsigned char)(str[i]) & 0xC0) != 0x80) {
				/* This value is not valid as a non-primary UTF8 octet */
				return(-1);
			}
		}

		return(r);
	}

	std::string wjUTF8Encode(char *value)
	{
		size_t	length = strlen(value);
		char		*next;
		char		*v;
		char		*e;
		size_t	l;
		char		esc[3];
		std::string	str;

		*(esc + 0) = '\\';
		*(esc + 1) = '\0';
		*(esc + 2) = '\0';

		for (v = e = value; e < value + length; e++) {
			switch (*e) {
				case '\\':	*(esc + 1) = '\\';	break;
				case '"':	*(esc + 1) = '"';	break;
				case '\n':	*(esc + 1) = 'n';	break;
				case '\b':	*(esc + 1) = 'b';	break;
				case '\t':	*(esc + 1) = 't';	break;
				case '\v':	*(esc + 1) = 'v';	break;
				case '\f':	*(esc + 1) = 'f';	break;
				case '\r':	*(esc + 1) = 'r';	break;

				default:
					l = wjWUTF8CharSize(e, length - (e - value));
					
					if (l > 1 || (l == 1 && *e >= '\x20'))
					{
						/*
							*e is the primary octect of a multi-octet UTF8
							character.  The remaining characters have been verified
							as valid UTF8 and can be skipped.
						*/
						e += (l - 1);
					} 
					else if (l == 1)
					{
						/*
							*e is valid UTF8 but is not a printable character, and will be escaped before
							being sent, using the JSON-standard "\u00xx" form
						*/
						char	unicodeHex[sizeof("\\u0000")];
						
						next = v;
						while(next < e)
							str += *next++;

						sprintf(unicodeHex, "\\u00%02x", (unsigned char) *e);
						
						str += unicodeHex;

						v = e + 1;
					}
					else if (l < 0)
					{
						/*
							*e is not valid UTF8 data, and must be escaped before
							being sent. But JSON-standard does not give us a mechanism
							so we chose "\xhh" format because of its almost universal comprehension.
						*/
						char	nonUnicodeHex[sizeof("\\x00")];

						next = v;
						while(next < e)
							str += *next++;

						sprintf(nonUnicodeHex, "\\x%02x", (unsigned char) *e);
						
						str += nonUnicodeHex;

						v = e + 1;
					}
					continue;
			}

			next = v;
			while(next < e)
				str += *next++;

			v = e + 1;

			str += esc[0];
			str += esc[1];

			continue;
		}

		for (int j = 0; j < length - (v - value); j++)
			str += *(v + j);

		return str;
	}
///////// end of JSON encoder
#pragma warning(pop)

const std::string jsonPrefix = "\"";
const std::string jsonSuffix = "\"";

std::string APubnubComm::JSONify( const std::string& input, bool is_safe )
{
   std::string result = input;
   // 1. escape any weird characters
   if (!is_safe)
   {
      const size_t BUFLEN = 512;
      char buffer[BUFLEN];
      strncpy_s(buffer, input.length(), input.c_str(), BUFLEN);
      result = wjUTF8Encode(buffer);
   }
   // 2. add JSON string prefix and suffix (simple mode) "<stripped-string>"
   result = jsonPrefix + result + jsonSuffix;
   return result;
}

std::string APubnubComm::UnJSONify( const std::string& input )
{
   std::string result = input;
   // remove JSON string prefix and suffix
   if (input.length() > 2)
   {
      // verify 1st and last chars are ok
      bool t1 = (input.find(jsonPrefix) == 0);
      bool t2 = (input.rfind(jsonSuffix) == input.length());
      if (t1 && t2)
         result = std::string( input.begin() + 1, input.end() - 1 );
   }
   // now, do some magic decoding of UTF8 here (OOPS)
   return result;
}

