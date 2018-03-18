#include "stdafx.h"
#include "PubnubComm.h"
#include "ChannelInfo.h"
#include "RAII_CriticalSection.h"
#include <string>

#define PUBNUB_CALLBACK_API
extern "C" {
#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
#include "pubnub_timers.h" // getting the last time token
#include "pubnub_blocking_io.h" // setting non blocking io (not needed maybe?)
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
   //::InitializeCriticalSectionAndSpinCount(&cs, 0x400);
   //// spin count is how many loops to spin before actually waiting (on a multiprocesssor system)
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

#define NEWLIMS
#ifdef NEWLIMS
#undef max
#include <limits>
static const int MAXINT = std::numeric_limits<int>().max();
#else
#include <climits>
static const int MAXINT = INT_MAX;
#endif

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
   // pubnub_set_non_blocking_io(pContext); // v2.3.2 does this for callback api anyway
   if (res != PNR_OK)
      return false;
   if (!is_remote)
   {
      //const float factor = 2.0; // try to find upper limit by searching (MAXINT doesn't work, is seemingly ignored)
      const int tmout_msec = /*PUBNUB_DEFAULT_SUBSCRIBE_TIMEOUT*/MAXINT/*/factor*/; // or, 24.855 days for 32-bit int
      pubnub_set_transaction_timeout(pContext, tmout_msec);
      // if local, we kick off the first subscribe automatically (gets a time token)
      init_sub_pending = true; // makes sure we do a "real" subscribe too
         // NOTE: not needed, just handle empty responses (this is the time token under the hood)
      return Listen();
   } else {
      pubnub_set_transaction_timeout(pContext, PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT);
      // if remote, we need to fire up the publisher queueing mechanism for the channel
      pQueue = new PNChannelPublishQueueing();
   }
   return true;
}

//double tmtoken_2_secs(const char* tmtoken)
//{
//   std::string s = tmtoken;
//   result = 0.0;
//
//}

// start to listen on this channel if local
bool PNChannelInfo::Listen(void) const
{
   pubnub_res res;
   res = pubnub_subscribe(pContext, this->channelName.c_str(), NULL);
   if (res != PNR_STARTED)
      return false;
   const char* last_tmtoken = pubnub_last_time_token(pContext);
   int msec = pubnub_transaction_timeout_get(pContext);
   TRACE("LISTENING FOR %1.3lf sec, last token=%s\n", msec/1000.0, last_tmtoken);
   return true;
}

bool PNChannelInfo::Send(const char*message)
{
   if (pContext == nullptr)
      return false;
   std::string msg = PNChannelInfo::JSONify(message);

   // if the message queue is in BUSY state, it means we should send there (huh?)
   RAII_CriticalSection rcs(pQueue->GetCS()); // lock queued access
   this->SendOptTimeRequest();

   if (this->pQueue->isBusy())
   {
      return pQueue->push(msg.c_str());
   } else {
      this->pQueue->setBusy(true);
      return this->SendBare(msg.c_str());
   }
}

bool PNChannelInfo::SendBare(const char*message) const
{
   pubnub_res res;

   //RAII_CriticalSection acs(pService->GetCS()); // lock queued access
   res = pubnub_publish(pContext, this->channelName.c_str(), message);
   if (res != PNR_STARTED) {
      return false; // error reporting at higher level
   }
   return true;
}

void PNChannelInfo::PublishRetry()
{
   RAII_CriticalSection rcs(pQueue->GetCS()); // lock queued access
   // request to publish the next one
   pQueue->get_publish(this);
}

std::string PNChannelInfo::TimeToken(const char* input)
{
   // request to publish the next one
   std::string result = pQueue->get_ttok();
   if (input != nullptr)
      pQueue->set_ttok(input);
   return result;
}

void PNChannelInfo::SendOptTimeRequest()
{
   RAII_CriticalSection rcs(pQueue->GetCS()); // lock queued access
   // if context has no time token, ask for one
   // this should only happen on init() for the Remote (pub-only) side unless it fails to work
   std::string tkn( this->pQueue->get_ttok() );
   if ("" == tkn)
   {
      pubnub_time(this->pContext);
      pQueue->setBusy(true); // can no longer use the context
   }
}

void PNChannelInfo::ContinuePublishing()
{
   RAII_CriticalSection rcs(pQueue->GetCS()); // lock queued access
   // if the queue has more items, 
   if (pQueue->isBusy()) {
      // request to publish the next one
      pQueue->pop_publish(this);
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

