#pragma once

/* OPERATIONAL SCHEME
The class will maintain two channels for various purposes:
1. THe local channel will be the one to use for the remote to talk back to this class on
2. The remote channel will be the one to use to contact the remote end

Channels will be named by convention as follows:
   MSW-<custname>-<devicename>
where the <custname> is the assigned customer name (needs to be unique across all MSW users)
, AND
the <devicename> is assigned by the customer and set up in the configuration data

IN THIS VERSION:
1. On startup, the class subscribes its assigned local channel (config file) (PRIMARY/SECONDARY both)
   a. State is kDisconnected
   b. No need to pay attention to any messages coming in here
2. When Initialize() occurs, the user can supply a different local name
   a. Decide if this is permanent or just a temporary override
   b. If perm, need to update the config when diffs are detected
   c. If temp, just let it override here.
   d. If changes happened, Unsubscribe the old channel, and subscribe to the new one
3. When OpenLink() occurs, the class is given the name of a SECONDARY to contact...
   a. The remote channel name is calculated and saved
   b. The Contact message is published to the remote channel
   c. The subscriber pays attention to callback messages FROM THAT REMOTE ONLY, if needed
   d. State is kConnecting until the AckContact comes back
   e. On receiving AckContact, state is kConnected and further connection requests are ignored
4. CloseLink() disconnects any use of the remote channel
   a. State goes to kDisconnecting if needed, outbound conversations cease
   b. Unsubscribe the remote channel
   c. When successful, state goes to kDisconnected, no error
   d. If error unsubscribing, state must go to kDisconnected with error condition

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
   ConnectionStatus fLinked; // disconnected, connected, or in transition
   std::string customerName;
   std::string deviceName;
   std::string deviceUUID;
   std::string pubAPIKey;
   std::string subAPIKey;

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
   const char* GetConnectionName() const;
   const char* GetConnectionTypeName() const;
   const char* GetConnectionStateName() const;

   // get/set options to/from persistent storage (registry)
   void Read();
   void Write();
   void Override();

public:
   APubnubComm(AComm* pComm);
   ~APubnubComm(void);

   // read and configure various option settings (called after registry access is initialized)
   void Configure();

   /** set the handle of the window that will receive status
      notifications.
     */
   void SetParent(HWND parent);

   // is this a PRIMARY or a SECONDARY
   bool isPrimary() const { return fConnection == kPrimary; }
   bool isSecondary() const { return fConnection == kSecondary; }
 
   // is the remote link connected? it's a process...
   bool isDisconnected() const { return fLinked == kDisconnected; }
   bool isConnected() const { return fLinked >= kConnected; }
   bool isConversing() const { return fLinked >= kChatting; }

   bool isBusy() const; // operation in progress, check back later
 
   // Will set up the the channel link(s) to go online
   // PRIMARY: no channel setup, just setup and verify connections and remember device name
   // SECONDARY: will also setup receiver channel to listen on private device channel
   // returns false if immediate errors
   bool Initialize(const char* deviceName);
   void Deinitialize();

   // PRIMARY: will open up a sender link to a particular SECONDARY (may also configure receiver to listen for Responses)
   // SECONDARY: needs to get a request for this over the link, to configure the sender channel where to send Responses
   bool OpenLink(const char * remote_channel);
   void CloseLink();

   // PRIMARY: called to send a message via the interface to the SECONDARY
   void SendCommand(const char* message);

   // SECONDARY: called to dispatch any received message from the interface to the parent window
   void OnMessage(const char* message);

   // chat code taken from old implementation by Steve Cox
   void SendCmd(WPARAM cmd, int param) {::PostMessage(fParent, WM_COMMAND, MAKEWPARAM(cmd, param), 0);}
   void SendMsg(UINT msg, WPARAM wParam=0, LPARAM lParam=0) {::PostMessage(fParent, msg, wParam, lParam);}

};

