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

/*
SEND CHANNEL CLASS

States used:
   kNone - set at startup when context is allocated, but has no channel info yet
   kDisconnected - set when object is not online but has all needed info (key, UUID, OPT proxy settings)
   kIdle - set when object is ready to send messages but no transaction is in progress
   kDisconnecting - set when object is waiting for results of pubnub_cancel
   [kConnecting - set when object is waiting to go online (during Ping Test command, e.g.)]
   kBusy - set when publish transactions are in progress

Operations:
      ** on UI thread **
   ctor - allocates context, STATE( kNone )
   Setup() - sets the key and UUID; STATE( kNone => kDisconnected )
      NOTE: For Proxy settings protocol, see note in ReceiveChannel.cpp
   Init() - takes the channel online by setting the channel name; STATE( kDisconnected => kIdle )
      TBD: If a Ping Test is needed, perform that via transition STATE( kDisconnected => kConnecting )
   Deinit() - takes the channel offline by calling cancel() if an operation is in progress; 
      EITHER STATE( kIdle => kDisconnected ) OR STATE( kBusy => kDisconnecting )

      ** on alt.thread **
   OnCallback() - if no errors, will use the PQ to continue to publish the next data (no state change)
      - when finally all data in PQ is published, STATE( {kBusy, kConnecting} => kIdle )
      - if cancel was successful (considered error), STATE( kDisconnecting => kDisconnected )
      - if other errors, will implement the PublishRetry loop (no state change)

Async Waits Needed:
   Setup() - none
   Init() - only if a Ping Test is in progress (TBD)
   DeInit() - only if an operation is being canceled (not too likely except during file transfers or scrolling)

Use of UI interlock for these Async calls:
   The intent is that the pService object can have a callback extension too, that can coordinate replies from each channel.
   On Logon, that object will coordinate a call to pReceiver->Init(), and wants a callback when the listen is started.
   On Connect, it will coordinate calls to both channel Init()s, and needs to wait for both before notifying the UI of completion.

Use of the PQ Object:
   This is the part of the publishing process that involves queueing messages when in the kBusy state.
*/

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
   // ignore Init calls while disconnecting or busy, but post a deferred Init?
   if (IsBusy()) {
      return false;
   }

   // PRIMARY sender gets this at Connect time
   // SECONDARY sender gets this from remote PING command (for sending responses)
   this->channelName = cname;
   state = remchannel::kIdle;
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
      // set channel state until the callback gets results
      state = remchannel::kDisconnecting;
      // NOTE: since callback may run synchronously on this thread, set the state 1st
      pubnub_cancel(pContext);
      break;
   default:
      // nothing to cancel, we just go "offline" (TBD - do we forget the channel we were using here?)
      state = remchannel::kDisconnected;
      break;
   }
   return result;
}

bool SendChannel::IsBusy() const
{
   bool result = false;
   switch (this->state) {
   case remchannel::kBusy:
   case remchannel::kDisconnecting:
   case remchannel::kConnecting:
      result = true;
      break;
   default:
      break;
   }
   return result;
}

bool SendChannel::Send(const char*message)
{
   // ignore Send calls while disconnecting
   if (state == remchannel::kDisconnecting) {
      return false;
   }

   if (pContext == nullptr)
      return false;
   std::string msg = SendChannel::JSONify(message);

   // if the message queue is in BUSY state, it means we should send there
   RAII_CriticalSection rcs(pQueue->GetCS()); // lock queued access

   // NOTE: PQ::busy state is same as kBusy in the local state and is crit-section protected
   if (this->pQueue->isBusy())
   {
      return pQueue->push(msg.c_str());
   } else {
      this->state = remchannel::kBusy;
      this->pQueue->setBusy(true);
      this->pubRetryCount = 1;
      return this->SendBare(msg.c_str());
   }
}

bool SendChannel::SendBare(const char*message)
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
   // request to publish the same one (don't pop the queue)
   pQueue->get_publish(this);
}

void SendChannel::ContinuePublishing()
{
   RAII_CriticalSection rcs(pQueue->GetCS()); // lock queued access
   // if the queue has more items,
   // ACTUALLY, just test if the PQ is in BUSY mode or not
   if (pQueue->isBusy()) {
      // request to publish the next one
      pQueue->pop_publish(this);
   } else {
      // otherwise we're done and it's back to direct publishing mode
      pQueue->setBusy(false); // redundant! it's already tested false in the if stmt!
      state = remchannel::kIdle;
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
   result = remchannel::JSON_PREFIX + result + remchannel::JSON_SUFFIX;
   return result;
}

// EXPERIMENTAL: There is a bug in 2.3.2 (and later?) that publish transactions don't record their time tokens, unlike subscribes do.
// This is code to dive into the internals of the PN context and retrieve the reply buffer contents and parse it.
// HIGHLY DEPENDENT ON LIBRARY VERSION - BUG IS REPORTED, MODIFY HERE WHEN FIXED! THIS MAY EVENTUALLY STOP WORKING!!!
//extern "C" {
#include "pubnub_internal.h" // for experimental pub timetoken bugfix
#include "ReceiveChannel.h" // for UnJSONify()
//}
//#undef min
namespace {
   std::string LastTimeTokenFromReply( const pubnub_t* pContext )
   {
      static char repbuf[256];
      size_t len = min(256, pContext->core.http_content_len);
      memcpy_s(repbuf, 256, pContext->core.http_reply, len);
      repbuf[len] = 0;
      // There are now three strings in repbuf, end to end, followed by another null.
      // 1st is code number string, 2nd is error string, 3rd is time token string
      const char* str2 = repbuf + strlen(repbuf) + 1;
      const char* str3 = str2 + strlen(str2) + 1;
      //int code = atoi(&repbuf[1]);
      //const char* message = ReceiveChannel::UnJSONify(str2).c_str();
      std::string ttoken = ReceiveChannel::UnJSONify(str3);
      //TRACE("PN pub reply with time token was [%d,%s,%s]\n", code, message, ttoken.c_str());
      return ttoken;
   }
}

void SendChannel::OnPublishCallback(pubnub_res res)
{
   std::string op, ttok;
   const char *last_tmtoken = "";
   const char* cname = this->GetName();
   remchannel::result result = remchannel::kOK;

   // CUSTOM CALLBACK FOR PUBLISHERS
   op = pubnub_last_publish_result(this->pContext);
   op += pubnub_res_2_string(res);
   this->op_msg = op;
   ttok = LastTimeTokenFromReply(pContext);
   last_tmtoken = ttok.c_str();
   // implement publish retry loop here
   if (res == PNR_OK) {
      TRACE("Publish succeeded (c=%d), moving on...\n", this->pubRetryCount);
      this->pubRetryCount = 1;
   } else {
      TRACE("Publishing failed with code: %d ('%s')\n", res, op.c_str());
      switch (pubnub_should_retry(res)) {
      case pbccFalse:
         TRACE(" There is no benefit to retry\n");
         this->pubRetryCount = 1;
         result = remchannel::kError;
         break;
      case pbccTrue:
         TRACE(" Retrying #%d...\n", this->pubRetryCount);
         this->pubRetryCount++;
      case pbccNotSet:
         TRACE(" We decided not to retry since it could make things worse.\n");
         this->pubRetryCount = 1;
         result = remchannel::kError;
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
   else if (this->pubRetryCount == 1) {
      remchannel::state oldState = state;
      this->ContinuePublishing();
      // notify client of completion of async Logout w/o errors
      if (oldState == remchannel::kDisconnecting)
         pService->OnTransactionComplete(remchannel::kSender, result);
   }
}

void SendChannel::OnTimeCallback(pubnub_res /*res*/)
{
   //std::string op;
   //const char *last_tmtoken = "";
   //const char* cname = this->GetName();

   //// CUSTOM CALLBACK #2 FOR PUBLISHERS
   //if (res == PNR_OK)
   //{
   //   last_tmtoken = pubnub_get(this->pContext); // NOTE: this data does NOT seem to ever be saved in the context!
   //   this->TimeToken(last_tmtoken); // update it in the channel info
   //}
   //// call the time difference reporter here
   //ReportTimeDifference(last_tmtoken, PBTT_TIME, res, op, cname, 0);
   //// TBD: do error handling and decide if OK to continue
   //// continue with the actual publish message this precedes
   //this->ContinuePublishing();
}


