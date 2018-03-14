#pragma once

#include <string>

class APubnubComm;
class PubMessageQueue;

extern "C" {
#include "pubnub_api_types.h"
   void pn_printf(char* fmt, ...);
}

// fwd.ref to free functions used in callback API target
extern void pn_callback(pubnub_t* pn, pubnub_trans t, pubnub_res res, void* pData);
extern const char* GetPubnubTransactionName(pubnub_trans t);
extern const char* GetPubnubResultName(pubnub_res res);
extern void TRACE_LAST_ERROR(LPCSTR fname, DWORD line);

class PNChannelInfo {
public:
   std::string key;
   std::string key2;
   std::string channelName;
   bool is_remote;
   pubnub_t * pContext;
   APubnubComm * pService;
   std::string op_msg;
   bool init_sub_pending; // requires first sub for time token (see C SDK docs for subscribe() and elsewhere)
   PubMessageQueue* pQueue; // lightweight queueing for publish messages

   PNChannelInfo(APubnubComm *pSvc);
   ~PNChannelInfo();

   bool Init(bool is_primary);
   bool DeInit();
   void ContinuePublishing(); // for use by pubnub callback function

   // NOTE: in this simple class, none of its data is modified by these routines
   bool Send(const char* data) const;
   bool SendBare(const char* data) const; // for use by msg.queue (impl.detail)
   bool Listen() const;
   void OnMessage(const char* data) const;

   const char* GetTypeName() const;
   // NOTE: 'safe' commands do not contain any escapable JSON string characters or non-ASCII chars
   static std::string JSONify( const std::string& input, bool is_safe=true );
   static std::string UnJSONify( const std::string& input );
private:
   PNChannelInfo(const PNChannelInfo& other);
   PNChannelInfo& operator=(const PNChannelInfo& other);
};
