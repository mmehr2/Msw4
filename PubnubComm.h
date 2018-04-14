#pragma once

/* OPERATIONAL SCHEME
The class will maintain two channels for various purposes:
1. The Receiver channel will be the one to use for listening for any remote communications to this machine
2. The Sender channel will be the one to use to contact the remote machine

Channels will be named by convention as follows:
   MSW-<custname>-<devicename>
where the <custname> is the assigned customer name (needs to be unique across all MSW users)
, AND
the <devicename> is assigned by the customer and set up in the configuration data

IN THIS VERSION:
1. On startup, the class sets up info but does not start communications (PRIMARY/SECONDARY both)
   a. State is kDisconnected
   b. No need to pay attention to any messages coming in here (would cause excess traffic)
2. When Login() occurs, the user can supply a different local name
   a. Decide if this is permanent or just a temporary override
   b. If perm, need to update the config when diffs are detected
   c. If temp, just let it override here.
   d. PRIMARY - no operation occurs (except configuring Receiver with new channel name)
   e. SECONDARY - Receiver enters subscribe loop listening for messages
 * f. If changes happened, may need to Unsubscribe the old channel, and subscribe to the new one
   g. State goes to kConnected immediately (no async ops)
3. PRIMARY: When OpenLink() occurs, the class is given the name of a SECONDARY to contact...
   a. The remote channel name is calculated and saved
   b. TBD The Contact message is published to the Sender channel
   c. The Receiver pays attention to a single callback message, entering a single subscribe request w/o looping
   d. State is kLinking until the AckContact comes back
   e. On receiving AckContact, state is kChatting
   f. SECONDARY: Receiver gets the Contact message, configures its Sender channel and sends back the AckContact in reply
   g. SECONDARY state goes to kChatting and further connection requests from other PRIMARY machines are ignored
4. PRIMARY: CloseLink() disconnects any use of the remote channel
   a. Any outgoing messages to Sender are canceled.
   b. At same time, Unsubscribe the Receiver channel if in use for a Response wait
   c. Wait for Receiver cancel to complete (if any)
   d. TBD Send the Disconnect message to the Sender channel to release the SECONDARY
   e. Wait for both Sender and Receiver ops to complete
   f. When successful, state goes to kConnected, no error
   g. If errors on R or S, object must decide if state returns to kChatting or moves to kConnected
   h. SECONDARY: Receiver gets Disconnect message, forgets about the Sender channel (cancel any publishes and state=>kUnlinking)
   i. SECONDARY: When done, state goes to kConnected and further Contact requests are allowed (ignore cancel errors?)
5. When Logout() occurs, the shutdown of links should happen.
   a. PRIMARY: If links are paired (kChatting), CloseLink() must be called first, and wait for the completion.
   b. Once we are in kConnected state, we can Deinit() both Sender and Receiver. No need to wait. (SECONDARY stays kConnected)
   c. SECONDARY: If logging out is initiated on the Secondary, and if the link is kChatting, the pairing must be removed first.
      c1. The Sender must send the Disconnect Response message to the PRIMARY and wait for the publish callback.
      c2. When it comes in, state goes to kConnected and logout can continue
   d. Then the Receiver subscribe loop must be canceled and then wait for completion.
   e. SECONDARY state goes to kDisconnected once the sub is canceled OK.
   f. Logout can fail if the Sender Disconnect fails to publish, or the Receiver cancel fails to work. State changes?

NOTE: All async replies that post status changes must be funneled through the fComm object before being posted to the parent window.
   This will allow fComm to change its state accordingly as well. The function that does this is @@@TBD@@@.
*/

#include <afx.h> // for HWND
#include "RemoteCommon.h"
#include <string>
#include "PubBufferTransfer.h"

class AComm; // fwd.refs
class SendChannel;
class ReceiveChannel;

extern "C" {
#include "pubnub_api_types.h"
}

class APubnubComm
{
   AComm* fComm;
   HWND fParent;
   ConnectionType fConnection; // Primary or Secondary
   ConnectionStatus fLinked; // state: disconnected, connected, or in transition, etc.

   // the following are persistent settings
   std::string customerName;
   std::string deviceName;
   std::string deviceUUID;
   std::string pubAPIKey;
   std::string subAPIKey;

   int fOperation; // what transaction we're working on (AComm::OpType)
   int statusCode; // latest transaction result, for reporting to the UI level, using AComm::Status codes
   std::string statusMessage; // string form of the above with details
   time_t cmdStartTime; // for timing execution of contact command (and others)
   time_t cmdLocalEndTime; // when pub op returns
   time_t cmdRemoteEndTime; // when sub op returns (round trip complete)
   time_t cmdEndTime; // when transaction ends (theoretically same as RemoteEnd, but ...)
   int contactCode; // result of Contact operation (OK, busy-reject, etc. - use AComm::Status codes)
   CStringA fPreferences; // store the string version of the app's preferences for transfer between machines

   // inbound pubnub channel info
   // mainly used by Secondary to listen for incoming Command messages
   // Primary will only this to listen for Response messages, if we implement those
   ReceiveChannel* pReceiver;

   // outbound channel info
   // this is mainly opened by Primary to send to a particular Secondary
   // if we use Secondary response messages, it would be used for sending those
   SendChannel* pSender;

   // object to use for script transfers (Primary: sender, Secondary: receiver)
   PNBufferTransfer* pBuffer; // bulk transfer interface

   // no need to copy or assign - singleton
   APubnubComm(const APubnubComm&);
   void operator=(const APubnubComm&);

   // helpers
   std::string MakeChannelName( const std::string& deviceName ) const;
   bool IsSameCompany( const std::string& channel_name ) const;
   const char* GetConnectionName() const;
   const char* GetConnectionTypeName() const;
   const char* GetConnectionStateName() const;
   const char* GetOperationName() const;
   void SetState( ConnectionStatus newState, int reason=0 )
   {
      statusCode = reason;
      fLinked = newState;
   }

   // get/set options to/from persistent storage (registry)
   void Read();
   void Write();
   bool ReadOverrideFile(const char* fileName); // returns true if any changes made to persistent settings
   void StoreMessage(bool clear, char* fmt, ...); // for logging plus tracing, saves to statusMessage
   const char* FormatCommand( int opCode, int arg1 = (-1), int arg2 =(-1), const std::string& argS = "" );
   const char* FormatOperation(int opCode); // format a conventional operation message
   
public:
   APubnubComm(AComm* pComm);
   ~APubnubComm(void);

   // read and configure various option settings (called after registry access is initialized)
   void Configure();

   /** set the handle of the window that will receive status
      notifications.
     */
   void SetParent(HWND parent);
   HWND GetParent() const { return fParent; }

   // is this a PRIMARY or a SECONDARY
   bool isPrimary() const { return fConnection == kPrimary; }
   bool isSecondary() const { return fConnection == kSecondary; }
 
   // is the remote link connected? it's a process...
   bool isDisconnected() const { return fLinked == kDisconnected; }
   bool isConnected() const { return fLinked >= kConnected; }
   bool isConversing() const { return fLinked >= kChatting; }
   CString GetConnectedName() const; // ID of the linked conversation or machine

   bool isBusy() const; // operation in progress, check back later
   int GetStatusCode() const; // report most-recently-sent status code (ONLY CALL IF NOT BUSY)
   bool isSuccessful() const; // report if most-recently-sent status code means a successful operation (ONLY CALL IF NOT BUSY)
   CString GetLastMessage() const; // most recently set status message (ONLY CALL IF NOT BUSY)
   double GetCommandTimeSecs() const;
   int GetContactCode() const; // report most-recently-received results of Contact command (primary to secondary)
 
   // Will set up the the channel link(s) to go online
   // PRIMARY: no channel setup, just setup and verify connections and remember device name
   // SECONDARY: will also setup receiver channel to listen on private device channel
   // returns false if immediate errors; check isBusy() to determine async operation
   bool Login(const char* asDeviceName); // STATE: kDisonnected -> kConnecting -> kConnected
   void Logout(); // STATE: kConnected -> kDisconnecting -> kDisonnected

   // PRIMARY: will open up a sender link to a particular SECONDARY (may also configure receiver to listen for Responses)
   // (SECONDARY: will execute the corresponding state changes as part of received commands)
   // returns false if immediate errors; check isBusy() to determine async operation
   bool OpenLink(const char * remote_channel); // STATE: kConnected -> kLinking -> kChatting
   void CloseLink(); // STATE: kChatting -> kUnlinking -> kConnected

   // PRIMARY: will start/stop execution of scroll mode (activates/deactivates the Receiver listener loop)
   // (SECONDARY: will execute the corresponding state changes as part of received commands)
   bool StartScrollMode(); // STATE: kChatting -> kBusy(w.op=kScrollOn) -> kScrolling
   void StopScrollMode(); // STATE: kScrolling -> kBusy(w.op=kScrollOff) -> kChatting

   // PRIMARY: called to send a message via the interface to the SECONDARY
   void SendCommand(const char* message);
   void SendOperation(int opCode); // send a conventional operation message
   // same, but go busy and imply a transaction wait until done
   bool SendCommandBusy(int opCode); // STATE: kChatting -> kBusy(w.op=...) -> kChatting
   void SendStatusReport() const; // UI can get the current op/status to be re-sent

   // SECONDARY: called to dispatch any received message from the interface to the parent window
   void OnMessage(const char* message);

   // called by Sender/Receiver callback when async operation (transaction start/cancel) completes
   void OnTransactionComplete(remchannel::type which, remchannel::result what);

   // message response routines
   void OnContactMessage( int onOff, const std::string& channel_name );
   void OnPreferences( const std::string& prefs );

   // Post a result message to the parent window (for UI action)
   // NOTE: this is based on chat code taken from old implementation by Steve Cox
   void SendCmd(WPARAM cmd, int param) {::PostMessage(fParent, WM_COMMAND, MAKEWPARAM(cmd, param), 0);}
   void SendMsg(UINT msg, WPARAM wParam=0, LPARAM lParam=0) {::PostMessage(fParent, msg, wParam, lParam);}

};

