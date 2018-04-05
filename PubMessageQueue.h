#pragma once

#include <deque>
#include <string>

#include <afx.h>

class SendChannel; // fwd.ref.

class PNChannelPublishQueueing {

   bool busy_flag; // PROTECTED BY CS - DO NOT ACCESS DIRECTLY
   CRITICAL_SECTION cs; // protect multiple-access writers from >1 thread
   std::deque<std::string> thePQ; // message queue
      // NOTE: Since PQ is only used internally, synchronization isn't needed
      // public clients must go through a message post to the thread message queue
   std::string last_timetoken; // since the pubnub context doesn't seem to save this on pub.requests

public:
   PNChannelPublishQueueing();
   ~PNChannelPublishQueueing();

   // PROTECTION: external locking usage (needs better design)
   LPCRITICAL_SECTION GetCS() { return &cs; }

   void setBusy(bool newValue); // used by both threads
   bool isBusy(); // used by both threads

   bool push(const char* data); // used by client class
   bool /*pop*/pop_publish(SendChannel* pDest); // used by callback routine from Pubnub
   bool /*get*/get_publish(SendChannel* pDest); // used by callback routine from Pubnub
   bool trigger_publish(SendChannel* pDest); // used for pubnub_time() triggering and similar non-pubsub events


private:
   void sendNextCommand(SendChannel* pWhere, bool retry); // used by private thread

};
