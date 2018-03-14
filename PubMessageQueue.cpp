#include "stdafx.h"
#include "PubMessageQueue.h"
#include "ChannelInfo.h"
//#include "PubnubComm.h"
#include "RAII_CriticalSection.h"

// PUBLISHING MESSAGE QUEUE CODE

/*
The idea here is to queue any messages that try to be published while a Pubnub publish transaction is in progress.
This should actually forward to the ChannelInfo object for most details.
The plan:
   > First publish goes out via regular pubnub function
      > It also sets BUSY=1 at that point
   > If a message is to be sent while BUSY==0, same thing happens
   > If however BUSY != 0 on publish, the message is placed in the PQ (publish queue)
      > Mechanism: post WM_QUEUE_SAVE_MSG to the thread, which will:
         > Save the message in the PQ (deque, into the write end)
   > When the main callback receives a completion code (PNR_OK), it tells the PQ to send the next message
      > It does this by posting a WM_QUEUE_SEND_MSG message to the thread, which will on response to that:
         > If !PQ.empty, it also sends the pubnub_publish() call for it and pulls it off the PQ
         > If PQ.empty, it sets BUSY=0 (is there any callback continuation needed for the Remote object here?)
Since the PQ is private to the thread, it doesn't need to be synchronzed.
The only sync needs to be over use of the BUSY flag, if it's a variable.
What if it's a Windows EVENT? Consider... same issue, right? ...

ISSUE: Creating the thread's message queue
This can be done by the publish mechanism looping until PostThreadMessage() succeeds, two calls should suffice.
However if there's a delay, this implies using a Ping message at channel connection time to make sure this is done once.
Subsequent calls should succeed on the first try. This is a Windows issue, NOT a Pubnub one.

*/
extern "C" static UINT InternalThreadFunction(LPVOID param); // fwd.ref

PubMessageQueue::PubMessageQueue()
   : pThread(nullptr)
   , busy_flag(false)
{
   ::InitializeCriticalSectionAndSpinCount(&cs, 0x400);
   // spin count is how many loops to spin before actually waiting (on a multiprocesssor system)
   // set up the queueing mechanism - create a thread for private queueing, no UI required
   pThread = ::AfxBeginThread(InternalThreadFunction, this);
   if (!pThread) {
      TRACE("UNABLE TO CREATE PRIVATE THREAD FOR PUBLISH QUEUEING!\n");
      TRACE_LAST_ERROR(__FILE__, __LINE__ - 1);
   }
}

extern "C" static UINT InternalThreadFunction(LPVOID param)
{
   // interface to Win API needs to be extern "C"; just call the class function here
   PubMessageQueue* pComm = reinterpret_cast<PubMessageQueue*>(param);
   PubMessageQueue::threadFunction(pComm);
   return 0;
}

PubMessageQueue::~PubMessageQueue()
{
   delete pThread;
   pThread = nullptr;

   ::DeleteCriticalSection(&cs);
}

void PubMessageQueue::init()
{
   // clear the queue (or wait until all messages are sent??)
   this->thePQ.clear();
}

void PubMessageQueue::deinit()
{
}

void PubMessageQueue::setBusy(bool newValue)
{
   RAII_CriticalSection rcs(&this->cs);
   this->busy_flag = newValue;
}

bool PubMessageQueue::isBusy()
{
   RAII_CriticalSection rcs(&this->cs);
   return this->busy_flag;
}

bool PubMessageQueue::postData(const char* msg)
{
   TRACE("PQ: Posting cmd %s\n", msg);

   //// we lied temporarily - just save a working copy since the original goes away
   //PubMessageQueue* pThis = const_cast<PubMessageQueue*>(this);
   //pThis->msg_being_posted = std::string(msg);

   //this->postMessage(PQ_SAVE_MSG, msg);
   this->saveCommand(msg);
   return true;
}

bool PubMessageQueue::postPublishRequest(PNChannelInfo* pDest) const
{
   TRACE("PQ: Posting pub.req.s\n");
   this->postMessage(PQ_SEND_MSG, (LPCSTR)(LPVOID)pDest);
   return true;
}

void PubMessageQueue::saveCommand(const char* command)
{
   RAII_CriticalSection rcs(&this->cs);
   std::string s(command); // copies the original
   this->thePQ.push_back(s);
   TRACE("PQ: Queueing cmd %s\n", command);
}

void PubMessageQueue::sendNextCommand(PNChannelInfo* pWhere)
{
   std::string command;
   RAII_CriticalSection rcs(&this->cs);
   if (this->thePQ.empty()) {
      // no more queued msgs - signal OK to go direct
      setBusy(false);
      TRACE("PQ: EMPTY - BUSY = OFF\n");
   } else {
      // get the next one to send from the queue
      const char* command = this->thePQ.front().c_str();
      // pop it
      this->thePQ.pop_front();
      TRACE("PQ: POP: Sending cmd %s to pci=%X\n", command, pWhere);
      // send the message to pubnub (but don't do the public version)
      pWhere->SendBare(command);
   }
}

// this is the private thread function called by the AComm object
// Its job is to block and wait until a message is posted, then wake up and handle it
// Supported messages:
//    PQ_SAVE_MSG - queues up a command string to be sent later (includes JSONification)
//    PQ_SEND_MSG - unqueues the first command string remaining and publishes it
/*static*/ void PubMessageQueue::threadFunction(PubMessageQueue* pComm)
{
   MSG msg;

   for (;;)
   {
      ::GetMessageA(&msg, NULL, PubMessageQueue::PQ_FIRST_MSG, PubMessageQueue::PQ_LAST_MSG); 
         // NOTE: this blocks thread until there's a msg in the queue
      switch (msg.message) {

      case PQ_SAVE_MSG:
         // save a string to the queue
         pComm->saveCommand( (LPCSTR)msg.lParam );
         break;

      case PQ_SEND_MSG:
         // publish next string from the queue
         pComm->sendNextCommand( (PNChannelInfo*)msg.lParam );
         break;

      }
   }
}

void PubMessageQueue::postMessage(UINT msgnum, const char* data) const
{
   if (pThread == nullptr)
   {
      TRACE("NO PRIVATE THREAD AVAILABLE FOR PUBLISH QUEUEING!\n");
      return;
   }
   // NOTE: loops until success - should only take at most 2 tries (1st sets up the msg.queue for the app)
   // see here: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644946(v=vs.85).aspx
   while ( !::PostThreadMessageA(this->pThread->m_nThreadID, msgnum, (WPARAM)0, (LPARAM)data) ) {
      ::Sleep(0);
   }
}

