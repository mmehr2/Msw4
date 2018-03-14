#pragma once

#include <deque>
#include <string>

#include <afx.h>

class PNChannelInfo; // fwd.ref.

class PubMessageQueue {

   CWinThread* pThread; // publish queue thread for PostThreadMessage() calls
   bool busy_flag; // PROTECTED BY CS - DO NOT ACCESS DIRECTLY
   CRITICAL_SECTION cs; // protect multiple-access writers from >1 thread
   std::deque<std::string> thePQ; // message queue
      // NOTE: Since PQ is only used internally, synchronization isn't needed
      // public clients must go through a message post to the thread message queue

public:
   PubMessageQueue();
   ~PubMessageQueue();

   void init();
   void deinit();

   void setBusy(bool newValue); // used by both threads
   bool isBusy(); // used by both threads

   static void threadFunction(PubMessageQueue* param); // used by AComm class via intermediaries (why??)

   bool postData(const char* data); // used by client class
   bool postPublishRequest(PNChannelInfo* pDest) const; // used by callback routine from Pubnub

private:
   void saveCommand(const char* data); // used by private thread
   void sendNextCommand(PNChannelInfo* pWhere); // used by private thread

   enum {
      PQ_FIRST_MSG = WM_USER + 16023,
         // WPARAM: unused
         // LPARAM: const char* ptr to command string to send
      PQ_SAVE_MSG = PQ_FIRST_MSG,
         // WPARAM: unused
         // LPARAM: pubnub_t* ptr to context to publish to
      PQ_SEND_MSG,
      PQ_LAST_MSG
   };
   void postMessage(UINT msgnum, const char* data = NULL) const; // used by main thread

};
