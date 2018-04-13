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

class SendChannel {
   remchannel::state state;
   std::string key;
   std::string key2;
   std::string deviceName;
   std::string channelName;
   std::string uuid;
   std::string op_msg;
   pubnub_t * pContext;
   APubnubComm * pService;
   PNChannelPublishQueueing* pQueue;
   int pubRetryCount;
   std::string overrideChannelName;
   std::string lastPubMessage;

public:
   SendChannel(APubnubComm *pSvc);
   ~SendChannel();

   // sets configuration info to allow channel operation
   void Setup(
      const std::string& uuid,
      const std::string& main_key,
      const std::string& aux_key);

   void SetDeviceName(const std::string& name) { deviceName = name; }
   const char* GetDeviceName() const { return deviceName.c_str(); }
   const char* GetName() const;
   bool isUnnamed() const { return channelName.empty(); }
   const char* GetLastMessage() const;

   // start online operation (sub listens, pub may send a ping)
   bool Init(const std::string& channelName); // sender
   // cancel operation on a channel, going offline when done
   bool DeInit();

   bool IsBusy() const;

   // for use by pubnub callback function (PROTECTED in case of UI thread usage)
   void ContinuePublishing(); 
   void PublishRetry();
   bool SendBare(const char* data); // UNPROTECTED

   // the callback extensions (will be called directly by the callback function)
   void OnPublishCallback(pubnub_res res);
   void OnTimeCallback(pubnub_res res);

   // For use by PubnubComm client (ultimately UI thread)
   bool Send(const char* data, const char* override_channel = nullptr);

   const char* GetTypeName() const;
   // NOTE: 'safe' commands do not contain any escapable JSON string characters or non-ASCII chars
   static std::string JSONify( const std::string& input, bool is_safe=true );
   //static std::string UnJSONify( const std::string& input );

private:
   // no copying or assignment of objects
   SendChannel(const SendChannel& other);
   SendChannel& operator=(const SendChannel& other);

};
