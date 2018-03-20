#pragma once

#include <string>
#include <ctime>

class APubnubComm;
class PNChannelPublishQueueing;
class PNBufferTransfer;

extern "C" {
#include "pubnub_api_types.h"
   void pn_printf(char* fmt, ...);
}

// fwd.ref to free functions used in callback API target
extern void pn_callback(pubnub_t* pn, pubnub_trans t, pubnub_res res, void* pData);
extern const char* GetPubnubTransactionName(pubnub_trans t);
extern const char* GetPubnubResultName(pubnub_res res);
extern void TRACE_LAST_ERROR(LPCSTR fname, DWORD line);
extern time_t get_local_timestamp();

class PNChannelInfo {
public:
   std::string key;
   std::string key2;
   std::string channelName;
   std::string deviceUUID;
   bool is_remote;
   pubnub_t * pContext;
   APubnubComm * pService;
   std::string op_msg;
   bool init_sub_pending; // requires first sub for time token (see C SDK docs for subscribe() and elsewhere)
   PNChannelPublishQueueing* pQueue; // lightweight queueing for publish messages
   PNBufferTransfer* pBuffer; // bulk transfer interface

   PNChannelInfo(APubnubComm *pSvc);
   ~PNChannelInfo();

   bool Init(bool is_primary);
   bool DeInit();

   // for use by pubnub callback function (PROTECTED in case of UI thread usage)
   void ContinuePublishing(); 
   void PublishRetry();
   bool SendBare(const char* data) const; // UNPROTECTED
   std::string TimeToken(const char* data = nullptr); // UNPROTECTED

   void OnMessage(const char* data) const; // for use by pubnub callback function (UNPROTECTED) (SUB)

   // For use by PubnubComm client (ultimately UI thread)
   bool Send(const char* data);
   bool Listen() const;

   const char* GetTypeName() const;
   // NOTE: 'safe' commands do not contain any escapable JSON string characters or non-ASCII chars
   static std::string JSONify( const std::string& input, bool is_safe=true );
   static std::string UnJSONify( const std::string& input );
private:
   PNChannelInfo(const PNChannelInfo& other);
   PNChannelInfo& operator=(const PNChannelInfo& other);

   void SendOptTimeRequest();
};
