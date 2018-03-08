#pragma once

// ACustomSocket command target

#pragma once

#include <afxsock.h>
#include <string>

class ACustomComm;

enum acs_result {
   acsOK = 0, // success or operation not necessary / already done
   acsPgmerror, // programming errors that shouldn't be encountered (VERIFY please)
   acsNetdown, // the network is down WSAENETDOWN
   acsResources, // no resources type: WSAEMFILE, WSAENOBUFS
   acsExpected, // protocol errors that are normal operation (WSAEWOULDBLOCK)
   acsTerminate, // protocol terminal conditions (end of operation) such as WSAETIMEDOUT
   acsUnknown, // none of the above (useful for chaining, don't give this to protocol)
};

class ACustomSocket : public CAsyncSocket
{
   ACustomComm* pComm;
   acs_result status;
   std::string address;
   int port;
   int wsaError;
public:
   ACustomSocket(ACustomComm* pCC);
   virtual ~ACustomSocket();

   //acs_result GetStatus() const { return status; }
   bool isOK() const { return status == acsOK; }
   bool isDown() const { return status == acsNetdown; }
   bool isProgrammerError() const { return status == acsPgmerror; }
   bool isResourceError() const { return status == acsResources; }
   bool isCannotComplete() const { return status == acsTerminate; }
   bool isContinuing() const { return status == acsExpected; }

   // helper function: get readable error code string from last recorded socket error
   CString getLastErrorString() const;
   int getLastErrorCode() const { return wsaError; }

   // specify the IP address and port of the other end (convenience function wrapper around Create())
   // assumes SOCK_STREAM (TCP/IP) and only flags of interest (e.g., no OOB notifications)
   void Configure(int port, const std::string& address = "");

   // wrapper for Connect(); supply the IP address of the remote end to connect to
   void ConnectX(const std::string& secondary_address);

   // wrapper for Listen() with error processing
   void ListenX();

   // utility function: classify WSA* error codes into error code enums
   static acs_result ClassifyError(int error);
   // utility function: get Windows error description for logging from WSA* code
   static CString GetWindowsErrorString(int wsaError);

protected:
   virtual void OnAccept(int nErrorCode); // listener: ready to call Accept() with new connection
   virtual void OnConnect(int nErrorCode); // caller: completed conection, OK or not, call GetLastError() to evaluate
   //virtual void OnReceive(int nErrorCode); // either: data available for retrieval by Receive()
   //virtual void OnSend(int nErrorCode); // either: buffer space is available to fill using Send() [continuation]
   //virtual void OnClose(int nErrorCode); // either: TBD capability for 2-stage Close notification (with WASEWOULDBLOCK) if too busy to close

public:
//private:
   // Helper Functions
   void PostProcessError(int res, int wsaCode = (-1)); // classify and set status
};


