// ACustomSocket.cpp : implementation file
//

#include "stdafx.h"
#include "Msw.h"
#include "CustomSocket.h"
#include "CustomComm.h"


// ACustomSocket

ACustomSocket::ACustomSocket(ACustomComm* pCC)
   : pComm(pCC)
   , status(acsOK)
   , address("")
   , port(0)
   , wsaError(0)
{
}

ACustomSocket::~ACustomSocket()
{
}


// ACustomSocket member functions

void ACustomSocket::Configure(int _port, const std::string& _address)
{
   port = _port;
   address = _address;
   CString adr = CA2T(_address.c_str());
   long flags = (/*FD_OOB |*/ FD_CLOSE);
   if (pComm->isPrimary()) {
      flags |= FD_READ | FD_WRITE | FD_CONNECT; // the primary Connector socket: reads, writes, connects
      TRACE(_T("Configuring Primary Connector socket for address %s:%d...\n"), (LPCTSTR)adr, port);
   } else {
      flags |= FD_ACCEPT; // the secondary Listener socket just accepts
      // NOTE: (the secondary Connector is auto-configured by the base class during OnAccept() when a new connection request comes in)
      TRACE(_T("Configuring Secondary Listener for port %d ..."), port);
   }
   LPCTSTR addr_param = adr == "" ? NULL : (LPCTSTR)adr;
   int res = this->Create((UINT)port, SOCK_STREAM, flags, addr_param);
   // if none, continue
   if (0 != res) {
      // to assure non-blocking mode, set it up here
      res = this->AsyncSelect(flags);
      if (0 != res) {
         status = acsOK;
         TRACE(_T(" AsyncSelect listener OK!\n"));
      } else {
         TRACE(_T(" AsyncSelect listener FAILED!\n"));
      }
   }
   // error processing for any error types
   this->PostProcessError(res);
}

void ACustomSocket::PostProcessError(int res, int wsaCode)
{
   if (0 == res) {
      // error: save and classify it
      this->wsaError = wsaCode != -1 ? wsaCode : (::GetLastError());
      this->status = ACustomSocket::ClassifyError(wsaError);
      TRACE(_T("Socket Config Error: type=%d, code=%d\n"), status, wsaError); // TBD: better strings needed here
   }
}

void ACustomSocket::ConnectX() {
   LPCTSTR aparm = (LPCTSTR)CA2T(address.c_str());
   int res = this->Connect( aparm, port );
   // error processing for any error types
   this->PostProcessError(res);
}

void ACustomSocket::ListenX() {
   int res = this->Listen( 1 ); // set queue backlog here
   // error processing for any error types
   this->PostProcessError(res);
}

void ACustomSocket::OnAccept(int nErrorCode) {
   // SECONDARY - forward accept event to the Comm protocol handler after classifying error codes
   ASSERT(pComm != NULL);
   this->PostProcessError(0, nErrorCode);
   if (this->status == acsUnknown)
      TRACE(_T("ACCEPT STATUS UNCLASSIFIED WSA ERROR - %d\n"), nErrorCode);
   else
      pComm->OnAccept();
}

acs_result ACustomSocket::ClassifyError(int error)
{
   acs_result res;
   switch (error) {
   case WSAENETDOWN:
      res = acsNetdown; // runtime: net is down
      break;
   case WSAEMFILE:
   case WSAENOBUFS:
      res = acsResources; // runtime: out of resources
      break;
   case WSANOTINITIALISED:
   case WSAEAFNOSUPPORT:
   case WSAEINPROGRESS:
   case WSAEPROTONOSUPPORT:
   case WSAEPROTOTYPE:
   case WSAESOCKTNOSUPPORT:
   case WSAEINVAL: // Create() error in args (address "" must be NULL)
       res = acsPgmerror; // programmer error, unexpected conditions
      break;
  default:
      res = acsUnknown; // classification failed at this level
      break;
   }
   return res;
}

CString ACustomSocket::GetWindowsErrorString(int nErrorCode)
{
   // stolen from Microsoft OnConnect() sample handler TBD - needs work!
   CString result;
   if (0 != nErrorCode)
   {
      switch(nErrorCode)
      {
         case WSAEADDRINUSE: 
            result.Format(_T("The specified address is already in use.\n"));
            break;
         case WSAEADDRNOTAVAIL: 
            result.Format(_T("The specified address is not available from ")
            _T("the local machine.\n"));
            break;
         case WSAEAFNOSUPPORT: 
            result.Format(_T("Addresses in the specified family cannot be ")
            _T("used with this socket.\n"));
            break;
         case WSAECONNREFUSED: 
            result.Format(_T("The attempt to connect was forcefully rejected.\n"));
            break;
         case WSAEDESTADDRREQ: 
            result.Format(_T("A destination address is required.\n"));
            break;
         case WSAEFAULT: 
            result.Format(_T("The lpSockAddrLen argument is incorrect.\n"));
            break;
         case WSAEINVAL: 
            result.Format(_T("The socket is already bound to an address.\n"));
            break;
         case WSAEISCONN: 
            result.Format(_T("The socket is already connected.\n"));
            break;
         case WSAEMFILE: 
            result.Format(_T("No more file descriptors are available.\n"));
            break;
         case WSAENETUNREACH: 
            result.Format(_T("The network cannot be reached from this host ")
            _T("at this time.\n"));
            break;
         case WSAENOBUFS: 
            result.Format(_T("No buffer space is available. The socket ")
               _T("cannot be connected.\n"));
            break;
         case WSAENOTCONN: 
            result.Format(_T("The socket is not connected.\n"));
            break;
         case WSAENOTSOCK: 
            result.Format(_T("The descriptor is a file, not a socket.\n"));
            break;
         case WSAETIMEDOUT: 
            result.Format(_T("The attempt to connect timed out without ")
               _T("establishing a connection. \n"));
            break;
         default:
            TCHAR szError[256];
            _stprintf_s(szError, _T("OnConnect error: %d"), nErrorCode);
            result.Format(szError);
            break;
      }
   }
   return result;
}
