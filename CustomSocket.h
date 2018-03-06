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
};

class ACustomSocket : public CAsyncSocket
{
   ACustomComm* pComm;
   acs_result status;
   CString address;
   int port;
public:
   ACustomSocket(ACustomComm* pCC);
   virtual ~ACustomSocket();

   //acs_result GetStatus() const { return status; }
   bool isOK() const { return acsOK == status; }
   bool isDown() const { return acsNetdown == status; }
   bool isProgrammerError() const { return acsPgmerror == status; }
   bool isResourceError() const { return acsResources == status; }

   // specify the IP address and port of the other end (convenience function wrapper around Create())
   // assumes SOCK_STREAM (TCP/IP) and only flags of interest (e.g., no OOB notifications)
   void Configure(int port, const std::string& address = "");

   // wrapper for Connect(); it already knows the port and address
   void ConnectX();

   void ListenX();

   // utility function: classify WSA* error codes into error code enums
   static acs_result ClassifyError(int error);
   // utility function: get readable error code string from error code enum
   // utility function: get Windows error description for logging from WSA* code
   static CString GetWindowsErrorString(int wsaError);

private:
   // Helper Functions
   void PostProcessError(int res); // classify and set status
};


