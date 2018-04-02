#include "stdafx.h"
#include "PubnubComm.h"
#include "ReceiveChannel.h"
#include "PubnubCallback.h"
#include <string>

#define PUBNUB_CALLBACK_API
extern "C" {
#define PUBNUB_LOG_PRINTF(...) pn_printf(__VA_ARGS__)
#include "pubnub_callback.h"
#include "pubnub_timers.h" // getting the last time token
#include "pubnub_blocking_io.h" // setting non blocking io (not needed maybe?)
}

/*
RECEIVE CHANNEL CLASS

States used:
   kNone - set at startup when context is allocated, but has no channel info yet
   kDisconnected - set when object is not online but has all needed info (channel name, key, and UUID, OPT proxy settings)
   kIdle - set when object is online listening for data (active subscribe on channel)
      NOTE: Channel remains in kIdle until disconnected.
   kDisconnecting - set when object is waiting for results of pubnub_cancel
   TBD - [kConnecting - set when object is waiting to go online (during proxy negotiations)]

Operations:
      ** on UI thread **
   ctor - allocates context, STATE( kNone )
   Setup() - sets the channel name, key and UUID; STATE( kNone => kDisconnected )
   Init() - takes the channel online by calling Listen(); STATE( kDisconnected => kIdle )
      [NOTE: TBD when proxy negotiation is needed, it can wait for it in Init() first using intermediate state kConnecting]
   Deinit() - takes the channel offline by calling cancel(); STATE( kIdle => kDisconnecting )
      This is a no-op if already offline.

      ** on alt.thread **
   OnCallback() - if no errors, will dispatch data and call Listen() again (no state change)
      - if cancel was successful (considered error), STATE( kDisconnecting => kDisconnected )
      - if other errors, TBD!

Async Waits Needed:
   Setup() - none
   Init() - none
   DeInit() - only if an operation is being canceled (if Idle at start)

MORE ON PROXY NEGOTIATIONS (Issue #10-B):
   This should be done at the system level (PubnubComm or higher).
   The call to pubnub_set_proxy_from_system() can block for quite a while, so it should be executed on a worker CWinThread.
   This implies the need for a way to do the state transition STATE( kConnecting => kIdle ) and kick off the Listen() call.
   Unfortunately, each context needs to have the info thus acquired, so a mechanism for sharing proxy settings needs to be added.
   Pubnub will be providing this on a future version of the code (afer 2.3.2) so check the development branch for how this will work.

   One good design would be to have the long call happen at startup time (if configured) by PubnubComm. Then each channel could be given the extra 
      information by the pService object. That object will need its own context to do this call on, so we can use a custom function at that level.
      Or perhaps a simple ProxyChannel object can encapsulate the sequences.
   In any case, it seems that if pService says proxy info is needed, we can hold off the transition to kDisconnected until both sets of info are
      available (channel name/UUID/key and proxy settings). There would be two Setup calls, each would set its own flag, and only if both flags 
      were set would it transition to kDisconnected state (on the second of the two calls).
   And of course, the Init() call would fail if both pieces of info weren't available. So defnitely check the return value of Init().
*/

ReceiveChannel::ReceiveChannel(APubnubComm *pSvc) 
   : state(remchannel::kNone)
   , key("demo")
   , channelName("")
   , uuid("")
   , pContext(nullptr)
   , pService(pSvc)
   , op_msg("")
   , init_sub_pending(true)
{
   //::InitializeCriticalSectionAndSpinCount(&cs, 0x400);
   //// spin count is how many loops to spin before actually waiting (on a multiprocesssor system)
   // we want the same lifetime between ChannelInfo and pubnub_context
   pContext = pubnub_alloc();
}

ReceiveChannel::~ReceiveChannel() 
{
   if (pContext)
      pubnub_free(pContext);
   pContext = nullptr;
}

const char* ReceiveChannel::GetTypeName() const
{
   return "RECEIVER";
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

void ReceiveChannel::Setup(
   const std::string& deviceUUID,
   const std::string& sub_key)
{
   this->key = sub_key;
   this->uuid = deviceUUID;
   pubnub_init(pContext, "", sub_key.c_str()); // receiver subscribes on this context only
   //const float factor = 2.0; // try to find upper limit by searching (MAXINT doesn't work, is seemingly ignored)
   const int tmout_msec = /*PUBNUB_DEFAULT_SUBSCRIBE_TIMEOUT*/MAXINT/*/factor*/; // or, 24.855 days for 32-bit int
   pubnub_set_transaction_timeout(pContext, tmout_msec);
   pubnub_set_uuid(pContext, deviceUUID.c_str());
   pubnub_register_callback(pContext, &pn_callback, (void*) this);
   if (state == remchannel::kNone)
      state = remchannel::kDisconnected;
   // else what? it's possible to use this for reconfiguration
}

bool ReceiveChannel::Init(void)
{
   if (state == remchannel::kNone)
      return false;
   // SECONDARY receiver gets this at Login time
   // PRIMARY receiver gets this at Connect time (or whenever a response is desired)
   // for all calls after the first on a new context, set transaction timeouts and either listen (local) or return
   // if local, we kick off the first subscribe automatically (gets a time token)
   this->init_sub_pending = true; // makes sure we do a "real" subscribe too
      // NOTE: not needed, just handle empty responses (this is the time token under the hood)
   return Listen();
}

// the job of this should be to close any connection. We should not need to free the context here, just the dtor.
bool ReceiveChannel::DeInit()
{
   if (state == remchannel::kDisconnecting) {
      // no need to send this more than once
      return true;
   }

   pubnub_cancel(pContext);
   // set Comm state to kDisconnecting until the callback gets results
   state = remchannel::kDisconnecting;
   return true;
}

// start to listen on this channel
bool ReceiveChannel::Listen(void)
{
   pubnub_res res;
   res = pubnub_subscribe(pContext, this->channelName.c_str(), NULL);
   if (res != PNR_STARTED)
      return false;
   const char* last_tmtoken = pubnub_last_time_token(pContext);
   int msec = pubnub_transaction_timeout_get(pContext);
   TRACE("LISTENING FOR %1.3lf sec, last token=%s\n", msec/1000.0, last_tmtoken);
   state = remchannel::kIdle;
   return true;
}

void ReceiveChannel::OnMessage(const char* data)
{
   // coming in on the subscriber thread
   if (pService) {
      // strip the JSON-ness (OK for simple strings anyway)
      std::string inner_data = UnJSONify(data);
      // send the message to the service layer
      pService->OnMessage(inner_data.c_str());
   }
}

std::string ReceiveChannel::UnJSONify( const std::string& input )
{
   std::string result = input;
   // remove JSON string prefix and suffix
   if (input.length() > 2)
   {
      // verify 1st and last chars are ok
      bool t1 = (input.find(remchannel::JSON_PREFIX) == 0);
      bool t2 = (input.rfind(remchannel::JSON_SUFFIX) == input.length() - 1);
      if (t1 && t2)
         result = std::string( input.begin() + 1, input.end() - 1 );
   }
   // now, do some magic decoding of UTF8 here (OOPS)
   return result;
}

void ReceiveChannel::OnSubscribeCallback(pubnub_res res)
{
   std::string op;
   const char *data = "";
   int msgctr = 0;
   std::string sep = "";
   const char* cname = this->GetName();

   // CUSTOM CALLBACK FOR SUBSCRIBERS
   if (res != PNR_OK) {
      // check for errors:
      // TIMEOUT - listen again
      // disconnect
      // other errors - notify an error handler, then listen again OR replace context?
      if (res == PNR_CANCELLED) {
         TRACE("SUB OPERATION CANCELLED.\n");
         if (state == remchannel::kDisconnecting) {
            state = remchannel::kDisconnected;
            TRACE("CHANNEL DISCONNECTED.\n");
         }
      }
      return;
   } else if (this->init_sub_pending) {
      // call Listen() again now that we have the initial time token for this context
      this->init_sub_pending = false;
      this->Listen();
      return;
   }
   // get all the data if OK
   while (nullptr != (data = pubnub_get(this->pContext)))
   {
      // dispatch the received data
      // after a reconnect, there might be several
      this->OnMessage(data);

      // stats: accumulate a list and count of messages processed here
      std::string sd(data);
      op += sd + sep;
      ++msgctr;
      sep = "; ";
   }
   op = "Rx{" + op + "}";
   // no more data to be read here (res is PNR_OK)
   // call the time difference reporter here
   const char* last_tmtoken = pubnub_last_time_token(this->pContext); // aseems to be vailable for sub transacs only
   ReportTimeDifference(last_tmtoken, PBTT_SUBSCRIBE, res, op, cname, msgctr);

   // TBD: do final error handling and decide if OK to continue
   // then reopen an active subscribe() to start the next data transaction
   this->Listen();
}
