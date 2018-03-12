#include "stdafx.h"
#include "PubnubComm.h"
#include "ChannelInfo.h"

#define PUBNUB_CALLBACK_API
extern "C" {
#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
}

PNChannelInfo::PNChannelInfo(/*pubnub_t * pC*/) 
   : key("demo")
   , key2("demo")
   , channelName("")
   , is_remote(false)
   , pContext(nullptr)
   , pService(nullptr)
   , op_msg("")
   , init_sub_pending(true)
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
      return false;
   if (!is_remote)
   {
      return Listen();
   }
   return true;
}

// start to listen on this channel if local
bool PNChannelInfo::Listen(void)
{
   pubnub_res res;
   res = pubnub_subscribe(pContext, this->channelName.c_str(), NULL);
   if (res != PNR_STARTED)
      return false;
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


void PNChannelInfo::OnMessage(const char* data)
{
   // coming in on the subscriber thread
   if (pService) {
      // strip the JSON-ness (OK for simple strings anyway)
      std::string inner_data = UnJSONify(data);
      // critical section code here? kick it upstairs
      pService->OnMessage(inner_data.c_str());
      // then reopen an active subscribe() to start the next data transaction
      Listen();
   }
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

void pn_callback(pubnub_t* pn, pubnub_trans trans, pubnub_res res, void* pData)
{
   // General debugging on the callback for now
   PNChannelInfo* pChannel = static_cast<PNChannelInfo*>(pData); // cannot be nullptr
   const char* cname = pChannel->channelName.c_str();// "NONE";
   std::string op;
   const char *data = "";
   int ctr = 0;
   std::string sep = "";
   switch (trans) {
   case PBTT_SUBSCRIBE:
      if (res != PNR_OK) {
         // check for errors:
         // TIMEOUT - listen again
         // disconnect
         // other errors - notify an error handler, then listen again OR replace context?
      } else if (pChannel->init_sub_pending) {
         // call Listen() again now that we have the time token for this context
      }
      // get all the data if OK
      while (nullptr != (data = pubnub_get(pn)))
      {
         // dispatch the received data
         // Q4V: is each one always delivered intact? or is it broken up? why multiples?
#ifndef SINGLE_DELIVERY_PER_SUB
         pChannel->OnMessage(data);
#endif
         std::string sd(data);
         op += sd + sep;
         ++ctr;
         sep = "; ";
      }
#ifdef SINGLE_DELIVERY_PER_SUB
      pChannel->OnMessage(op.c_str());
#endif
      op = "{" + op + "}";
      break;
   case PBTT_PUBLISH:
      op = pubnub_last_publish_result(pn);
      pChannel->op_msg += op;
      break;
   default:
      break;
   }
   TRACE("@*@_CB> %s IN %s ON: %s (T=%X)\n%s", 
      GetResultName(res), GetTransactionName(trans), cname, ::GetCurrentThreadId(), op.c_str());
   pChannel = nullptr; // DEBUG -- BREAKS CAN GO HERE
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

std::string PNChannelInfo::JSONify( const std::string& input, bool is_safe )
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

std::string PNChannelInfo::UnJSONify( const std::string& input )
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

