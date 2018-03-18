#include "stdafx.h"
//#include "PubnubComm.h"
#include "ChannelInfo.h"
#include "RAII_CriticalSection.h"

#define PUBNUB_CALLBACK_API
extern "C" {
#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
#include "pubnub_helper.h"
}

// PUBNUB IMPLEMENTATION (ASSOCIATED WITH PNChannelInfo CLASS)

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
   case PBTT_TIME:
      result = "TIME";
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

// NOTE: Windows file times are based on Jan 1 1601 UTC, not Jan 1 1970
static const int64_t conv1970from1601 = 116444736000000000LL;

void TimetToFileTime( time_t t, LPFILETIME pft )
{
   // FT = (t * 10^7) + CF
   LONGLONG xx = Int32x32To64(t, 10000000) + conv1970from1601;
   pft->dwLowDateTime = (DWORD) xx;
   pft->dwHighDateTime = xx >>32;
}

time_t FileTimetToTimetEx( FILETIME ft, time_t* t = NULL)
{
   // t = (FT - CF) / 10^7
   time_t result = ((int64_t)ft.dwHighDateTime << 32LL) + ft.dwLowDateTime; // FT
   result -= conv1970from1601;
   //result /= 10000000LL;
   if (t)
      *t = result;
   return result;
}

extern time_t get_local_timestamp()
{
   FILETIME ft;
   ::GetSystemTimeAsFileTime(&ft);
   time_t result = FileTimetToTimetEx(ft);
   return result;
}

static int continue_publish = 1;

void pn_callback(pubnub_t* pn, pubnub_trans trans, pubnub_res res, void* pData)
{
   // General debugging on the callback for now
   PNChannelInfo* pChannel = static_cast<PNChannelInfo*>(pData); // cannot be nullptr

   // since we are arriving on an independent library thread, protect access here
   //RAII_CriticalSection(pChannel->GetCS());
   
   const char* cname = pChannel->channelName.c_str();// "NONE";
   std::string op;
   const char *data = "";
   int msgctr = 0;
   std::string sep = "";
   //continue_publish = 1; // retry state
   const char* last_tmtoken = nullptr;
   switch (trans) {
   case PBTT_SUBSCRIBE:
      continue_publish = 0;
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
      op += pubnub_res_2_string(res);
      pChannel->op_msg = op;
      // implement retry loop here
      if (res == PNR_OK) {
         TRACE("Publish succeeded (c=%d), moving on...\n", continue_publish);
         continue_publish = 1;
      } else {
         switch (pubnub_should_retry(res)) {
         case pbccFalse:
            TRACE("Publish failed UNEXPLAINED, but we decided not to retry\n");
            continue_publish = 1;
            break;
         case pbccTrue:
            TRACE("Publishing failed with code: %d ('%s')\nRetrying #%d...\n", res, pubnub_res_2_string(res), continue_publish);
            continue_publish++;
         case pbccNotSet:
            TRACE("Publish failed, but we decided not to retry\n");
            continue_publish = 1;
            break;
         }
      }
      break;
   case PBTT_TIME:
      continue_publish = 1;
      if (res == PNR_OK)
      {
         last_tmtoken = pubnub_get(pn); // NOTE: this data does NOT seem to ever be saved in the context!
         pChannel->TimeToken(last_tmtoken); // update it in the channel info
      }
      break;
   default:
      break;
   }
   if (last_tmtoken == nullptr) 
      last_tmtoken = pubnub_last_time_token(pn);
   std::string ltt = last_tmtoken;
   if (pChannel->is_remote) {
      ltt = pChannel->TimeToken();
      last_tmtoken = ltt.c_str();
   }
   time_t pn_time = _atoi64(last_tmtoken);
   time_t lts = get_local_timestamp();
   static char lts_str[32];
   time_t lts_secs = lts / 1e7;
   ctime_s(lts_str, 32, &lts_secs);
   lts_str[24] = '\0'; // get rid of that final CR
   double td = difftime( pn_time, lts) / 1e7; // converted to sec
   TRACE("@*@_CB> %s IN %s ON: %s (T=%X)%s(c=%d,cp=%d,t=%lld(%s),lt=%lld(%s),diff=%1.7lf)\n", 
      GetPubnubResultName(res), GetPubnubTransactionName(trans), cname, ::GetCurrentThreadId(), op.c_str(), 
      msgctr, continue_publish, pn_time, last_tmtoken, lts, lts_str, td);
   if (continue_publish >= 2)
      pChannel->PublishRetry();
   else if (continue_publish == 1)
      pChannel->ContinuePublishing();
   pChannel = nullptr; // DEBUG -- BREAKS CAN GO HERE
}


