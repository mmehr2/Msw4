#pragma once

#include <ctime>
#include <string>

extern "C" {
#include "pubnub_api_types.h"
   void pn_printf(char* fmt, ...);
}

// fwd.ref to free functions used in callback API target
extern void pn_callback(pubnub_t* pn, pubnub_trans t, pubnub_res res, void* pData);
extern const char* GetPubnubTransactionName(pubnub_trans t);
extern const char* GetPubnubResultName(pubnub_res res);
//extern void TRACE_LAST_ERROR(LPCSTR fname, DWORD line);
extern time_t get_local_timestamp();
extern void ReportTimeDifference(const char* last_tmtoken, 
   pubnub_trans trans, 
   pubnub_res res, 
   const std::string& op, 
   const char* cname, int msgctr);
extern void GetCallbackCounters(int* pTotal=nullptr, 
   int* pTotalSubs=nullptr, int* pTotalPubs=nullptr, 
   int* pTotalTimes=nullptr, int* pTotalNones=nullptr);
