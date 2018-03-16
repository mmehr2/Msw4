#pragma once

#include <deque>
#include <string>

#include <afx.h>

class PNChannelInfo; // fwd.ref.

class PubMessageQueue {

   //CWinThread* pThread; // publish queue thread for PostThreadMessage() calls
   bool busy_flag; // PROTECTED BY CS - DO NOT ACCESS DIRECTLY
   CRITICAL_SECTION cs; // protect multiple-access writers from >1 thread
   std::deque<std::string> thePQ; // message queue
      // NOTE: Since PQ is only used internally, synchronization isn't needed
      // public clients must go through a message post to the thread message queue

public:
   PubMessageQueue();
   ~PubMessageQueue();

   //void init();
   //void deinit();

   void setBusy(bool newValue); // used by both threads
   bool isBusy(); // used by both threads

   static void threadFunction(PubMessageQueue* param); // used by AComm class via intermediaries (why??)

   bool push(const char* data); // used by client class
   bool /*pop*/pop_publish(PNChannelInfo* pDest); // used by callback routine from Pubnub
   bool /*get*/get_publish(PNChannelInfo* pDest); // used by callback routine from Pubnub

private:
   void sendNextCommand(PNChannelInfo* pWhere, bool retry); // used by private thread

};
