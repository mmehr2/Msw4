#pragma once

#include <string>
//#include <ctime>
#include "RemoteCommon.h"

class APubnubComm;

class ReceiveChannel {
   remchannel::state state;
   std::string key;
   //std::string key2;
   std::string deviceName;
   std::string channelName;
   std::string uuid;
   std::string op_msg;
   pubnub_t * pContext;
   APubnubComm * pService;
   bool init_sub_pending; // requires first sub for time token (see C SDK docs for subscribe() and elsewhere)
   unsigned int waitTimeSecs; // whether the channel will loop on subscribes (0) or just wait for one (>0), set by last Listen()

public:
   ReceiveChannel(APubnubComm *pSvc);
   ~ReceiveChannel();

   // sets configuration info to allow channel operation
   void Setup(const std::string& uuid,
      const std::string& sub_key);

   void SetName(const std::string& name) { channelName = name; }
   const char* GetName() const { return channelName.c_str(); }
   void SetDeviceName(const std::string& name) { deviceName = name; }
   const char* GetDeviceName() const { return deviceName.c_str(); }
   bool isUnnamed() const { return channelName.empty(); }
   const char* GetLastMessage() const;

   // start online operation (sub listens, pub may send a ping)
   bool Init(); // receiver
   // cancel operation on a channel, going offline when done
   bool DeInit();

   bool IsBusy() const;

   void OnMessage(const char* data); // for use by OnSubscribeCallback() (UNPROTECTED) (SUB)
   void OnSubscribeCallback(pubnub_res res); // for use by pubnub callback function (UNPROTECTED) (SUB)

   // For use by PubnubComm client (ultimately UI thread)
   bool Listen(unsigned int wait_secs = 0);

   const char* GetTypeName() const;
   // NOTE: 'safe' commands do not contain any escapable JSON string characters or non-ASCII chars
   //static std::string JSONify( const std::string& input, bool is_safe=true );
   static std::string UnJSONify( const std::string& input );

private:
   // no copying or assignment of objects
   ReceiveChannel(const ReceiveChannel& other);
   ReceiveChannel& operator=(const ReceiveChannel& other);

};
