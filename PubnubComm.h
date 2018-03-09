#pragma once

#include <afx.h> // for HWND
#include "RemoteCommon.h"
#include <string>

class AComm;

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


struct PNChannelInfo {
   std::string key;
   void * pContext;
   std::string channelName;
   void * pConnection; //?? what waits for data??

   PNChannelInfo();
};

class APubnubComm
{
   AComm* fComm;
   HWND fParent;
   ConnectionType fConnection; // Primary or Secondary
   ConnectionStatus fLinked; // disconnected, connected, or in transition
   std::string customerName;

   // inbound pubnub channel info
   PNChannelInfo local;

   // outbound channel info
   PNChannelInfo remote;

   // no need to copy or assign - singleton
   APubnubComm(const APubnubComm&);
   void operator=(const APubnubComm&);

   // helpers
   std::string MakeChannelName( const std::string& deviceName );
   static std::string JSONify( const std::string& input );
   static std::string UnJSONify( const std::string& input );
   const char* GetConnectionName() const;

public:
   APubnubComm(AComm* pComm);
   ~APubnubComm(void);

   /** set the handle of the window that will receive status
      notifications.
     */
   void SetParent(HWND parent);

   // is this a PRIMARY, SECONDARY, or we haven't decided
   bool isPrimary() { return fConnection == kPrimary; }
   bool isSecondary() { return fConnection == kSecondary; }
   bool isSet() { return fConnection != kNotSet; }

   // is the remote link connected? it's a process...
   bool isConnected() { return fLinked == kConnected; }
   bool isDisconnected() { return fLinked == kDisconnected; }
   bool isConnecting() { return fLinked == kConnecting; }

   // Will set up the connection to either publish or subscribe
   // returns false if immediate errors
   bool Initialize(bool as_primary, const char* publicName);
   void Deinitialize();

  // PRIMARY: will open up a link to a particular SECONDARY
   // . PRIMARY - will set up a communications socket
   // SECONDARY: will start the listening process
   // . SECONDARY - will set up a listening socket
   // TEST VERSION: this will actually cause a change into a PRIMARY operation
   // Will delete any secondary listener socket, then configure a connection using the given address, customary protocol port
   // State machine operation may also add a round trip string exchange test
   bool OpenLink(const char * address);
   bool CloseLink();

   // PRIMARY: called to send a message via the interface to the SECONDARY
   void SendCommand(const char* message);

   // SECONDARY: called to dispatch any received message from the interface to the parent window
   void OnMessage(const char* message);

};

