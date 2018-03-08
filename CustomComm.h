#pragma once

#include "RemoteCommon.h" // in common between protocols, for use by AComm public interface
#include <string>

enum {
   PROTOCOL_PORT = 8826, // agreed by customers for default direct comm, TBD needs to be reconfigurable later
};

class AComm;
class ACustomSocket;

class ACustomComm
{
   AComm* fComm;
   HWND fParent;
   ConnectionType fConnection;
   ConnectionStatus fLinked;
   ACustomSocket* pSocket; // communications socket (either)
   ACustomSocket* pListenerSocket; // SECONDARY: listener socket
   int protocol_port; // set to allow reconfiguration later
   std::string local_address; // local, configured on machine
   int last_sockerror; // most recent error code from socket operation OR 0 if OK

   // really, it's a singleton; disallow assignment and copying
   ACustomComm(const ACustomComm&);
   ACustomComm& operator=(const ACustomComm&);

public:

   // regular ctor and non-virtual dtor
   ACustomComm(AComm* pComm);
   ~ACustomComm(void);

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

   // Will set the type of connection to use unless kNoChange is specified
   // Will be called at program startup; returns false if immediate errors, else true
   // TEST VERSION: assumed to be secondary at startup
   // ALSO: call AfxSocketInit() to init the subsystem (P & S both)
   bool Initialize(ConnectionType conn_type, const char* local_address = NULL, int port = (-1));
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
   void SendMessage(const char* message);

   // SECONDARY: called to handle events from underlying socket
   void OnAccept(); // SECONDARY: ready to set up new connection (incoming request event)
   void OnConnect(); // PRIMARY: completed conection attempt, OK or not, check w underlying socket
   //void OnReceive(); // SECONDARY: data available for retrieval by Receive()
   //void OnSend(); // PRIMARY: buffer space is available to fill using Send() [continuation]

   // SECONDARY: called to dispatch any received message from the interface to the parent window
   void OnMessage(const char* message);

   int getLastCommErrorCode() const;
   CString getLastCommErrorString() const;

};

