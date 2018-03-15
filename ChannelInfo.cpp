#include "stdafx.h"
#include "PubnubComm.h"
#include "ChannelInfo.h"
#include "RAII_CriticalSection.h"

#define PUBNUB_CALLBACK_API
extern "C" {
#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
#include "pubnub_timers.h"
}

PNChannelInfo::PNChannelInfo(APubnubComm *pSvc) 
   : key("demo")
   , key2("demo")
   , channelName("")
   , is_remote(false)
   , pContext(nullptr)
   , pService(pSvc)
   , op_msg("")
   , init_sub_pending(true)
   , pQueue(nullptr)
{
}

PNChannelInfo::~PNChannelInfo() 
{
   DeInit();
}

const char* PNChannelInfo::GetTypeName() const
{
   if (this->is_remote)
      return "REMOTE";
   else
      return "LOCAL";
}

bool PNChannelInfo::Init(bool is_publisher)
{
   pubnub_res res;
   if (pContext)
      pubnub_free(pContext);
   pContext = pubnub_alloc();
   is_remote = is_publisher;
   if (is_remote)
      pubnub_init(pContext, key.c_str(), key2.c_str()); // remote publishes on this channel only
   else
      pubnub_init(pContext, "", key.c_str()); // local subscribes on this channel only
   res = pubnub_register_callback(pContext, &pn_callback, (void*) this);
   if (res != PNR_OK)
      return false;
   if (!is_remote)
   {
      // if local, we kick off the first subscribe automatically (gets a time token)
      init_sub_pending = true; // makes sure we do a "real" subscribe too
      return Listen();
   } else {
      // if remote, we need to fire up the publisher queueing mechanism for the channel
      pQueue = new PubMessageQueue();
   }
   return true;
}

// start to listen on this channel if local
bool PNChannelInfo::Listen(void) const
{
   pubnub_res res;
   res = pubnub_subscribe(pContext, this->channelName.c_str(), NULL);
   if (res != PNR_STARTED)
      return false;
   int msec = pubnub_transaction_timeout_get(pContext);
   TRACE("LISTENING FOR %1.3lf sec\n", msec/1000.0);
   return true;
}

bool PNChannelInfo::Send(const char*message) const
{
   if (pContext == nullptr)
      return false;
   std::string msg = PNChannelInfo::JSONify(message);
   // if the message queue is in BUSY state, it means we should send there (huh?)
   if (this->pQueue->isBusy())
   {
      return pQueue->postData(msg.c_str());
   } else {
      this->pQueue->setBusy(true);
      return this->SendBare(msg.c_str());
   }
}

bool PNChannelInfo::SendBare(const char*message) const
{
   pubnub_res res;

   //RAII_CriticalSection acs(pService->GetCS());
   res = pubnub_publish(pContext, this->channelName.c_str(), message);
   if (res != PNR_STARTED) {
      return false; // error reporting at higher level
   }
   return true;
}

void PNChannelInfo::PublishRetry()
{
   // request to publish the next one
   pQueue->postPublishRetryRequest(this);
}

void PNChannelInfo::ContinuePublishing()
{
   // if the queue has more items, 
   if (pQueue->isBusy()) {
      // request to publish the next one
      pQueue->postPublishRequest(this);
   } else {
      // otherwise we're done and it's back to direct publishing mode
      pQueue->setBusy(false);
   }
}

void PNChannelInfo::OnMessage(const char* data) const
{
   // coming in on the subscriber thread
   if (pService) {
      // strip the JSON-ness (OK for simple strings anyway)
      std::string inner_data = UnJSONify(data);
      // send the message to the service layer
      pService->OnMessage(inner_data.c_str());
   }
}

bool PNChannelInfo::DeInit()
{
   if (pContext) {
      pubnub_free(pContext);
      pContext = nullptr;
   }
   delete pQueue;
   pQueue = nullptr;
   return true;
}

const std::string jsonPrefix = "\"";
const std::string jsonSuffix = "\"";

std::string PNChannelInfo::JSONify( const std::string& input, bool is_safe )
{
   std::string result = input;
   // 1. escape any weird characters
   if (!is_safe)
   {
      //const size_t BUFLEN = 512;
      //char buffer[BUFLEN];
      //strncpy_s(buffer, input.length(), input.c_str(), BUFLEN);
      //result = wjUTF8Encode(buffer);
   }
   // 2. add JSON string prefix and suffix (simple mode) "<stripped-string>"
   result = jsonPrefix + result + jsonSuffix;
   return result;
}

std::string PNChannelInfo::UnJSONify( const std::string& input )
{
   std::string result = input;
   // remove JSON string prefix and suffix
   if (input.length() > 2)
   {
      // verify 1st and last chars are ok
      bool t1 = (input.find(jsonPrefix) == 0);
      bool t2 = (input.rfind(jsonSuffix) == input.length() - 1);
      if (t1 && t2)
         result = std::string( input.begin() + 1, input.end() - 1 );
   }
   // now, do some magic decoding of UTF8 here (OOPS)
   return result;
}

