#pragma once

#include <string>
#include <ctime>
#include "RemoteCommon.h"

class APubnubComm;
class PNChannelPublishQueueing;

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

class SendChannel {
public:
   remchannel::state state;
   std::string key;
   std::string key2;
   std::string deviceUUID;
   std::string op_msg;
private:
   pubnub_t * pContext;
   APubnubComm * pService;
   std::string channelName;
   PNChannelPublishQueueing* pQueue;
   int pubRetryCount;

public:
   SendChannel(APubnubComm *pSvc);
   ~SendChannel();

   // sets configuration info to allow channel operation
   void Setup(
      const std::string& uuid,
      const std::string& main_key,
      const std::string& aux_key);

   void SetName(const std::string& name) { channelName = name; }
   const char* GetName() const { return channelName.c_str(); }
   bool isUnnamed() const { return channelName.empty(); }

   // start online operation (sub listens, pub may send a ping)
   bool Init(const std::string& channelName); // sender
   // cancel operation on a channel, going offline when done
   bool DeInit();

   bool IsBusy() const;

   // for use by pubnub callback function (PROTECTED in case of UI thread usage)
   void ContinuePublishing(); 
   void PublishRetry();
   bool SendBare(const char* data) const; // UNPROTECTED
   std::string TimeToken(const char* data = nullptr); // UNPROTECTED

   // the callback extensions (will be called directly by the callback function)
   void OnPublishCallback(pubnub_res res);
   void OnTimeCallback(pubnub_res res);

   // For use by PubnubComm client (ultimately UI thread)
   bool Send(const char* data);

   const char* GetTypeName() const;
   // NOTE: 'safe' commands do not contain any escapable JSON string characters or non-ASCII chars
   static std::string JSONify( const std::string& input, bool is_safe=true );
   //static std::string UnJSONify( const std::string& input );

private:
   // no copying or assignment of objects
   SendChannel(const SendChannel& other);
   SendChannel& operator=(const SendChannel& other);

   void SendOptTimeRequest();
};
