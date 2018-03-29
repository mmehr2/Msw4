#include "stdafx.h"
#include "PubMessageQueue.h"
#include "SendChannel.h"
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

BIG ISSUE: Scrolling is a Modal Dialog and only dispatches Window Messages (see the ::DispatchMessage() call in the middle of ScrollDialog.cpp).
If we use PostThreadMessage(), we need a thread-specific hook.
(See here: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644946(v=vs.85).aspx:
   "Messages sent by PostThreadMessage are not associated with a window. As a general rule, messages that are not 
   associated with a window cannot be dispatched by the DispatchMessage function. Therefore, if the recipient thread 
   is in a modal loop (as used by MessageBox or DialogBox), the messages will be lost. To intercept thread messages 
   while in a modal loop, use a thread-specific hook.")
Check out the T-S Hook... dire warnings about slowing the system down, CBT_* stuff, needs a separate DLL, etc.
We would use a WH_MSGFILTER hook, and it needs to be called by CallMsgFilter() in the ScrollDialog's modal loop. Check the bottom of 
   this page: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644959(v=vs.85).aspx

   What if we use a hidden window instead? Less fuss, is it easy? Well, we need a better version of AfxBeginThread, more stack space, ...
   And if we use SendMessage and variants, see discussions here also: https://blogs.msdn.microsoft.com/oldnewthing/20110304-00/?p=11303
   But, all we really want to do is PostMessage() so we should be good.. but don't get tempted! ;)
Sooo, hidden windows and making this a GUI thread... not really a good idea either!
   See discussion here: https://stackoverflow.com/questions/17340285/can-i-have-multiple-gui-threads-in-mfc
But the best way seems to be to add ONE message to the main app, process it during the Scroll Dialog (only?), and pack up the PQ_* code 
   in the WPARAM of that message. Then the Scroll Dialog handler must call the PQ::ThreadFunction with the for/GetMessage() removed.
   OR - we only do it this way during scrolling for those commands that happen during scrolling. The idea is to prevent the Callback thread 
   from Pubnub from directly executing this code (BUT WHY??)
      OK Explore the idea: CB() already is coming in asynchronously on another thread while the UI thread is doing its thing, so prob is
      running on another core.. why not have it do what it does best? Well, sync with data that the UI thread might use, for one.
      Would there be anything here that the UI thread would use? PRIMARY: more publishing, SECONDARY: more subscribe responses ... this is
      already covered, we ultimately do PostMessage() there. So why not for the PQ_ funcs here? Because we should just do the work locally.
      On the CB thread, just pop the queue and publish the next or same message. No fussing with Windows, already busy on the UI thread, 
      except that PRIMARY UI will ultimately be posting messages to the queue too.
      SCENARIO:
         >UI thread calls gComm->SendMessage() -- leads to the code that will PublishOrDefer(), mostly will defer (hit the queue, put msg)
         >CB comes in with reply OK, decides to Publish, hits the queue to get the next msg, so yeah, sync/protect the queue!
DECISION: OK, let's not use any messages here, just realize that the CB (pn_callback) thread will empty the PQ on the PRIMARY while the
   GUI thread is filling it, classic case. And on any modern processor with >1 core, these will be on separate cores too. Can we guarantee it?
   One more thought process, follow here: https://stackoverflow.com/questions/663958/how-to-control-which-core-a-process-runs-on
   So, we can't control which actual core we will run on, we can only hope that other apps will be shut off, but Windows services abound 
   so there is never a dedicated machine any more. Add another thread to the mix and deal with what you get, whether it's timeslicing or 
   multicore execution. All handled by the CRITICAL_SECTION code under the hood for us.

   Double-check that VC2010's std::deque is thread-safe though...
   Maps are discussed here: https://stackoverflow.com/questions/17601722/stl-map-implementation-in-visual-c-2010-and-thread-safety
   Looking for the deque - found this: https://blogs.msdn.microsoft.com/nativeconcurrency/2009/11/23/the-concurrent_queue-container-in-vs2010/
   So, Intel had a complicated implementation to do producer/consumer properly - try NOT to throw exceptions, but concurrent safety seems 
      very difficult. (Calls ScheduleTask internally? Needs custom debug visualizers? Oi!) And the simple std::queue class has not a clue!
   I think the complexity is due to making the object lockless (more performant) - the simple locked deque should be tried first.
UPDATED DECISION:
   OK I get it, protection at this level is too fine-grained.
   We don't need this class at all! Put the CS back into the ChannelInfo, along with the deque.
   One operation for the UI thread to push new messages onto the PQ OR the Net (PublishOrDefer)
   One operation for the CB thread to decide how to deferred-publish - using PublishPQ or PublishPQ_Retry
   They are atomic ops due to the CS, and the ONLY things that should be used by the callers.
   Internally, all the rest is fine.
UPDATE: It's fine, this is the difference in SendChannel for publishing (other can be specialized for subscribing, maybe?)
*/

/*

WARNING - THIS CLASS IS NOT THREAD-SAFE!
Intended to be used as a private inner class by SendChannel, it will eventually be subsumed there.

*/

PNChannelPublishQueueing::PNChannelPublishQueueing()
   : busy_flag(false)
   , last_timetoken("")
{
   ::InitializeCriticalSectionAndSpinCount(&cs, 0x400);
   // spin count is how many loops to spin before actually waiting (on a multiprocesssor system)
}

PNChannelPublishQueueing::~PNChannelPublishQueueing()
{
   ::DeleteCriticalSection(&cs);
}

void PNChannelPublishQueueing::setBusy(bool newValue)
{
   this->busy_flag = newValue;
}

bool PNChannelPublishQueueing::isBusy()
{
   return this->busy_flag;
}

bool PNChannelPublishQueueing::push(const char* msg)
{
   RAII_CriticalSection rcs(&this->cs);
   std::string s(msg); // copies the original
   this->thePQ.push_back(s);
   TRACE("PQ: Queueing cmd %s\n", msg);
   return true;
}

bool PNChannelPublishQueueing::pop_publish(SendChannel* pDest)
{
   this->sendNextCommand(pDest, false);
   return true;
}

bool PNChannelPublishQueueing::get_publish(SendChannel* pDest)
{
   this->sendNextCommand(pDest, true);
   return true;
}

bool PNChannelPublishQueueing::trigger_publish(SendChannel* /*pDest*/)
{
   this->setBusy(true); // can't use the context again even tho the queue may be empty
   return true;
}

void PNChannelPublishQueueing::sendNextCommand(SendChannel* pWhere, bool retry)
{
   std::string command;
   bool do_send = false;
   bool do_done = false;
   const char* cmdstr = "";
   command = "";
   RAII_CriticalSection rcs(&this->cs);

   if (!this->thePQ.empty()) {
         if (retry) {
         // non-empty queue, retry request
         // get the current one to send from the queue
         command = this->thePQ.front();
//         TRACE("PQXX> %s\n", command.c_str());
         do_send = true;
      } else {
         // non-empty queue, normal request
         // get the current one to send from the queue, if any
         command = this->thePQ.front();
         //TRACE("PQXX1> %s\n", command.c_str());
         this->thePQ.pop_front();
         //TRACE("PQXX2> %s\n", command.c_str());
         do_send = true;
      }
   } else if (retry) {
      // empty queue, retry request
      // this can happen in the trigger_publish() scenario if the queue is otherwise empty
      // ERROR
//      TRACE("PQ: RETRY REQUEST ON EMPTY QUEUE!\n");
   } else {
      // empty queue, normal request
//      TRACE("PQ: RETRY REQUEST ON EMPTY QUEUE!\n");
      do_done = true;
   }
   cmdstr = command.c_str();
   if (command.empty())
      do_send = false;
   else
      TRACE("PQ: %s: Sending cmd %s to pci=%X\n", retry ? "RETRY" : "POP", cmdstr, pWhere);
   // send the message to pubnub (but don't do the public version)
   if (do_send) {
      pWhere->SendBare(cmdstr);
   } else if (do_done) {
      // no more queued msgs - signal OK to go direct
      this->setBusy(false);
      TRACE("PQ: EMPTY - BUSY = OFF\n");
   }
}
