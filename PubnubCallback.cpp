#include "stdafx.h"
#include "PubnubCallback.h"
#include "SendChannel.h"
#include "ReceiveChannel.h"
//#include "RAII_CriticalSection.h"

#define PUBNUB_CALLBACK_API
extern "C" {
#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
#include "pubnub_helper.h"
}

// PUBNUB IMPLEMENTATION
namespace rem {

   const char* GetPubnubTransactionName(pubnub_trans t)
   {
      const char* result = "UNSUPPORTED";
      switch (t) {
      case PBTT_NONE:
         result = "NONE";
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
         result = "CANCELLED"; // normal cancellation return
         break;
      case PNR_STARTED:
         result = "PLS-WAIT"; // normal immediate transaction return
         break;
      case PNR_IN_PROGRESS:
         result = "BUSY"; // can't start another transaction while one is still in progress
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

   StringVector split( const char* data_, const char* delim )
   {
      StringVector result;
      CStringA data(data_);
      int pos = 0;
      CStringA token;
      for (token = data.Tokenize(delim, pos); !token.IsEmpty(); token = data.Tokenize(delim, pos)) {
         result.push_back( (LPCSTR)token );
      }
      return result;
   }

   std::string join( const StringVector& data_, const char* delim_ )
   {
      std::string result = "";
      const char* delim = "";
      for (StringVector::const_iterator it = data_.begin(); it != data_.end(); ++it) {
         result += delim;
         result += *it;
         delim = delim_;
      }
      return result;
   }

   // NOTE: Windows file times are based on Jan 1 1601 UTC, not Jan 1 1970
   const int64_t conv1970from1601 = 116444736000000000LL;

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

   time_t get_local_timestamp()
   {
      FILETIME ft;
      ::GetSystemTimeAsFileTime(&ft);
      time_t result = FileTimetToTimetEx(ft);
      return result;
   }

   std::string get_local_time_token()
   {
      time_t tstamp = get_local_timestamp();
      CStringA tt;
      tt.Format("%017lld", tstamp);
      return (LPCSTR)tt;
   }

   // remember most recent previous values of system and local hi-res timestamps (100ns precision or secs/10^7)
   static time_t last_localtime = 0;
   static time_t last_systemtime = 0;

   void ReportTimeDifference(const char* last_tmtoken, 
      pubnub_trans trans, 
      pubnub_res res, 
      const std::string& op, 
      const char* cname, int msgctr)
   {
      // time difference reporting feature TBD- needs work!
      std::string ltt = last_tmtoken;
      time_t pn_time = _atoi64(last_tmtoken); // most recent system time (hi-res)
      time_t lts = get_local_timestamp(); // most recent local time (hi-res)
      static char lts_str[32];
      time_t lts_secs = lts / 10000000; // 1e7
      ctime_s(lts_str, 32, &lts_secs);
      lts_str[24] = '\0'; // get rid of that final CR
      double td = difftime( pn_time, lts) / 1e7; // converted to sec
      double td1 = difftime( pn_time, last_systemtime ) / 1e7;
      double td2 = difftime( lts, last_localtime ) / 1e7;
      int cbCountTotal, cbCountSubs, cbCountPubs;
      GetCallbackCounters(&cbCountTotal, &cbCountSubs, &cbCountPubs);
      TRACE("@*@_CB> %s IN %s ON: %s (T=%X)%s(c=%d[TT=%d/TTs=%d/TTp=%d],t=%s,lt=%s,diff=%1.7lf,sysdiff=%1.7lf,locdiff=%1.7lf)\n", 
         GetPubnubResultName(res), GetPubnubTransactionName(trans), cname, ::GetCurrentThreadId(), op.c_str(), 
         msgctr, cbCountTotal, cbCountSubs, cbCountPubs, last_tmtoken, lts_str, td, td1, td2);
      // update statistics for next time
      last_systemtime = pn_time;
      last_localtime = lts;
   }

   int callback_counter = 0;
   int callback_counter_sub = 0;
   int callback_counter_pub = 0;
   int callback_counter_time = 0;
   int callback_counter_none = 0;

   void GetCallbackCounters(int* pTotal, int* pTotalSubs, int* pTotalPubs, int* pTotalTimes, int* pTotalNones)
   {
      if (pTotal)
         *pTotal = callback_counter;
      if (pTotalSubs)
         *pTotalSubs = callback_counter_sub;
      if (pTotalPubs)
         *pTotalPubs = callback_counter_pub;
      if (pTotalTimes)
         *pTotalTimes = callback_counter_time;
      if (pTotalNones)
         *pTotalNones = callback_counter_none;
   }

   void pn_callback(pubnub_t* /*pn_context*/, pubnub_trans trans, pubnub_res res, void* pData)
   {
      // General debugging on the callback for now
      //PNChannelInfo* pChannel = static_cast<PNChannelInfo*>(pData); // cannot be nullptr

      // since we are arriving on an independent library thread, protect data used in these calls if needed

      callback_counter++;
      switch (trans) {
      case PBTT_SUBSCRIBE:
         callback_counter_sub++;
         {
            // CUSTOM CALLBACK FOR SUBSCRIBERS
            ReceiveChannel* pChannel = static_cast<ReceiveChannel*>(pData); // cannot be nullptr
            pChannel->OnSubscribeCallback(res);
         }
         break;
      case PBTT_PUBLISH:
         callback_counter_pub++;
         {
            // CUSTOM CALLBACK FOR PUBLISHERS
            SendChannel* pChannel = static_cast<SendChannel*>(pData); // cannot be nullptr
            pChannel->OnPublishCallback(res);
         }
         break;
      case PBTT_TIME:
         callback_counter_time++;
         {
            // CUSTOM CALLBACK #2 FOR PUBLISHERS
            SendChannel* pChannel = static_cast<SendChannel*>(pData); // cannot be nullptr
            pChannel->OnTimeCallback(res);
         }
         break;
      default:
         {
            // only real possibility here is a PBTT_NONE transaction
            // when do these happen?
            callback_counter_none++;
            TRACE("Received a NONE transaction #%s, result = %d ('%s').\n", callback_counter_none, res, pubnub_res_2_string(res));
         }
         break;
      }

      //pChannel = nullptr; // DEBUG -- BREAKS CAN GO HERE
   }

} // namespace rem
