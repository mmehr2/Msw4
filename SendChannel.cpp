#include "stdafx.h"
#include "PubnubComm.h"
#include "SendChannel.h"
#include "RAII_CriticalSection.h"
#include <string>
#include "PubMessageQueue.h"
#include "PubnubCallback.h"

#define PUBNUB_CALLBACK_API
extern "C" {
#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
#include "pubnub_timers.h" // getting the last time token
#include "pubnub_helper.h" // various publish helpers
}

SendChannel::SendChannel(APubnubComm *pSvc) 
   : state(remchannel::kNone)
   , key("demo")
   , key2("demo")
   , deviceUUID("")
   , op_msg("")
   , pContext(nullptr)
   , pService(pSvc)
   , channelName("")
   , pQueue(nullptr)
   , pubRetryCount(0)
{
   //::InitializeCriticalSectionAndSpinCount(&cs, 0x400);
   //// spin count is how many loops to spin before actually waiting (on a multiprocesssor system)
   // we want the same lifetime between ChannelInfo and pubnub_context
   pContext = pubnub_alloc();
}

SendChannel::~SendChannel() 
{
   if (pContext)
      pubnub_free(pContext);
   pContext = nullptr;
   delete pQueue;
   pQueue = nullptr;
}

const char* SendChannel::GetTypeName() const
{
   return "SENDER";
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

void SendChannel::Setup(
   const std::string& deviceUUID,
   const std::string& main_key,
   const std::string& aux_key)
{
   pubnub_init(pContext, main_key.c_str(), aux_key.c_str()); // sender publishes on this channel only
   pubnub_set_transaction_timeout(pContext, PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT);
   // if sender, we also need to fire up the publisher queueing mechanism for the channel
   if (!pQueue)
      pQueue = new PNChannelPublishQueueing();
   pubnub_set_uuid(pContext, deviceUUID.c_str());
   pubnub_register_callback(pContext, &pn_callback, (void*) this);
   if (state == remchannel::kNone)
      state = remchannel::kDisconnected;
   // else what? it's possible to use this for reconfiguration
}

bool SendChannel::Init(const std::string& cname)
{
   // PRIMARY sender gets this at Connect time
   // SECONDARY sender gets this from remote PING command (for sending responses)
   this->channelName = cname;
   return true;
}

// the job of this should be to close any connection. We should not need to free the context here, just the dtor.
bool SendChannel::DeInit()
{
   bool result = true;
   switch (state) {
   case remchannel::kDisconnecting:
      result = false; // wait until result done
      break;
   case remchannel::kBusy:
      // op in progress: cancel it
      pubnub_cancel(pContext);
      // set channel state until the callback gets results
      state = remchannel::kDisconnecting;
      break;
   default:
      // nothing to cancel, we just go "offline" (TBD - do we forget the channel we were using here?)
      state = remchannel::kDisconnected;
      break;
   }
   return result;
}

bool SendChannel::Send(const char*message)
{
   if (pContext == nullptr)
      return false;
   std::string msg = SendChannel::JSONify(message);

   // if the message queue is in BUSY state, it means we should send there
   RAII_CriticalSection rcs(pQueue->GetCS()); // lock queued access
   this->SendOptTimeRequest();

   if (this->pQueue->isBusy())
   {
      return pQueue->push(msg.c_str());
   } else {
      this->pQueue->setBusy(true);
      this->pubRetryCount = 1;
      return this->SendBare(msg.c_str());
   }
}

bool SendChannel::SendBare(const char*message) const
{
   pubnub_res res;

   //RAII_CriticalSection acs(pService->GetCS()); // lock queued access
   res = pubnub_publish(pContext, this->channelName.c_str(), message);
   if (res != PNR_STARTED) {
      return false; // error reporting at higher level
   }
   return true;
}

void SendChannel::PublishRetry()
{
   RAII_CriticalSection rcs(pQueue->GetCS()); // lock queued access
   // request to publish the next one
   pQueue->get_publish(this);
}

std::string SendChannel::TimeToken(const char* input)
{
   // request to publish the next one
   std::string result = pQueue->get_ttok();
   if (input != nullptr)
      pQueue->set_ttok(input);
   return result;
}

void SendChannel::SendOptTimeRequest()
{
   RAII_CriticalSection rcs(pQueue->GetCS()); // lock queued access
   // if context has no time token, ask for one
   // this should only happen on init() for the Remote (pub-only) side unless it fails to work
   std::string tkn( this->pQueue->get_ttok() );
   if (tkn.empty())
   {
      pubnub_time(this->pContext);
      this->pubRetryCount = 1;
      pQueue->setBusy(true); // can no longer use the context
   }
}

void SendChannel::ContinuePublishing()
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

std::string SendChannel::JSONify( const std::string& input, bool is_safe )
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
   result = remchannel::JSON_PREFIX + result + remchannel::JSON_PREFIX;
   return result;
}

void SendChannel::OnPublishCallback(pubnub_res res)
{
   std::string op;
   const char *last_tmtoken = "";
   const char* cname = this->GetName();

   // CUSTOM CALLBACK FOR PUBLISHERS
   op = pubnub_last_publish_result(this->pContext);
   op += pubnub_res_2_string(res);
   this->op_msg = op;
   last_tmtoken = this->TimeToken().c_str();
   // implement publish retry loop here
   if (res == PNR_OK) {
      TRACE("Publish succeeded (c=%d), moving on...\n", this->pubRetryCount);
      this->pubRetryCount = 1;
   } else {
      TRACE("Publishing failed with code: %d ('%s')\n", res, pubnub_res_2_string(res));
      switch (pubnub_should_retry(res)) {
      case pbccFalse:
         TRACE(" There is no benefit to retry\n");
         this->pubRetryCount = 1;
         break;
      case pbccTrue:
         TRACE(" Retrying #%d...\n", res, pubnub_res_2_string(res), this->pubRetryCount);
         this->pubRetryCount++;
      case pbccNotSet:
         TRACE(" We decided not to retry since it could make things worse.\n");
         this->pubRetryCount = 1;
         break;
      }
   }
   // call the time difference reporter here
   ReportTimeDifference(last_tmtoken, PBTT_PUBLISH, res, op, cname, this->pubRetryCount);
   // TBD: do final error handling and decide if OK to continue
   // then decide how to continue
   // TBD: also limit MAX number of retries, or add a wait, or something
   if (this->pubRetryCount >= 2)
      this->PublishRetry();
   else if (this->pubRetryCount == 1)
      this->ContinuePublishing();
}

void SendChannel::OnTimeCallback(pubnub_res res)
{
   std::string op;
   const char *last_tmtoken = "";
   const char* cname = this->GetName();

   // CUSTOM CALLBACK #2 FOR PUBLISHERS
   if (res == PNR_OK)
   {
      last_tmtoken = pubnub_get(this->pContext); // NOTE: this data does NOT seem to ever be saved in the context!
      this->TimeToken(last_tmtoken); // update it in the channel info
   }
   // call the time difference reporter here
   ReportTimeDifference(last_tmtoken, PBTT_TIME, res, op, cname, 0);
   // TBD: do error handling and decide if OK to continue
   // continue with the actual publish message this precedes
   this->ContinuePublishing();
}


