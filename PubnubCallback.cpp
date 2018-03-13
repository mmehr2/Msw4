#include "stdafx.h"
#include "PubnubComm.h"
#include "ChannelInfo.h"

#define PUBNUB_CALLBACK_API
extern "C" {
#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
}

// PUBNUB IMPLEMENTATION

const char* GetPubnubTransactionName(pubnub_trans t)
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

const char* GetPubnubResultName(pubnub_res res)
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

   // since we are arriving on an independent library thread, protect access here
   RAII_CriticalSection(pChannel->pService->GetCS());
   
   const char* cname = pChannel->channelName.c_str();// "NONE";
   std::string op;
   const char *data = "";
   int msgctr = 0;
   std::string sep = "";
   switch (trans) {
   case PBTT_SUBSCRIBE:
      if (res != PNR_OK) {
         // check for errors:
         // TIMEOUT - listen again
         // disconnect
         // other errors - notify an error handler, then listen again OR replace context?
         break;
      } else if (pChannel->init_sub_pending) {
         // call Listen() again now that we have the initial time token for this context
         pChannel->init_sub_pending = false;
         pChannel->Listen();
         break;
      }
      // get all the data if OK
      while (nullptr != (data = pubnub_get(pn)))
      {
         // dispatch the received data
         // after a reconnect, there might be several
         pChannel->OnMessage(data);

         // stats: accumulate a list and count of messages processed here
         std::string sd(data);
         op += sd + sep;
         ++msgctr;
         sep = "; ";
      }
      op = "Rx{" + op + "}";
      // no more data to be read here (res is PNR_OK)
      // then reopen an active subscribe() to start the next data transaction
      pChannel->Listen();
      break;
   case PBTT_PUBLISH:
      op = pubnub_last_publish_result(pn);
      pChannel->op_msg = op;
      break;
   default:
      break;
   }
   TRACE("@*@_CB> %s IN %s ON: %s (T=%X)%s(c=%d)\n", 
      GetPubnubResultName(res), GetPubnubTransactionName(trans), cname, ::GetCurrentThreadId(), op.c_str(), msgctr);
   pChannel = nullptr; // DEBUG -- BREAKS CAN GO HERE
}

#ifdef EXTERN_JSON_ENCODE
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

#endif //def EXTERN_JSON_ENCODE

