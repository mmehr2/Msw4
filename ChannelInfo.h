#pragma once

#include <string>

class APubnubComm;

extern "C" {
#include "pubnub_api_types.h"
   void pn_printf(char* fmt, ...);
}

// fwd.ref to free function used as callback API target
void pn_callback(pubnub_t* pn, pubnub_trans t, pubnub_res res, void* pData);

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

   PNChannelInfo(/*pubnub_t * pC*/);
   ~PNChannelInfo();

   bool Init(bool is_primary);
   bool DeInit();

   bool Send(const std::string& message);
   bool Listen();
   void OnMessage(const char* data);

   const char* GetTypeName();
   // NOTE: 'safe' commands do not contain any escapable JSON string characters or non-ASCII chars
   static std::string JSONify( const std::string& input, bool is_safe=true );
   static std::string UnJSONify( const std::string& input );
private:
   PNChannelInfo(const PNChannelInfo& other);
   PNChannelInfo& operator=(const PNChannelInfo& other);
};
