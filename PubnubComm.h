#pragma once

#include <afx.h> // for HWND
#include "RemoteCommon.h"
#include <string>

class AComm;
namespace pubnub {
   class context;
   class futres;
};

//struct InitInfo {
//   ConnectionType fContype;
//   std::string config_name;
//
//   InitInfo()
//      : fContype(fSecondary)
//      , config_name("")
//   {}
//};

class APubnubComm
{
   AComm* fComm;
   HWND fParent;
   ConnectionType fConnection;
   ConnectionStatus fLinked;
   std::string pubkey;
   std::string subkey;
   pubnub::context * pPCPublic;
   std::string publicChannelName; // "MSW.cname.dvcname" or ""
   pubnub::context * pPCChat;
   std::string chatChannelName; // "MSW.cname.dvcname.primaryname" or ""
//   pubnub::futres * pConnecter; // !=0 when awaiting input on reception channel
//   pubnub::futres * pConversation; // !=0 when awaiting input on chat channel (MSW.cname.dvcname.primaryname)
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

   // is the Primary/Secondary link connected? it's a process...
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
   static void OnMessage(const char* message);

};

