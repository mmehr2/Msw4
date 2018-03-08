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
         TRACE(_T(" AsyncSelect setup OK!\n"));
      } else {
         TRACE(_T(" AsyncSelect setup FAILED!\n"));
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

void ACustomSocket::ConnectX(const std::string& secondary_address) {
   CA2W wide(secondary_address.c_str());
   LPCTSTR aparm = wide.m_psz;
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

void ACustomSocket::OnConnect(int nErrorCode) {
   // SECONDARY - forward accept event to the Comm protocol handler after classifying error codes
   ASSERT(pComm != NULL);
   this->PostProcessError(0, nErrorCode);
   if (this->status == acsUnknown)
      TRACE(_T("CONNECT STATUS UNCLASSIFIED WSA ERROR - %d\n"), nErrorCode);
   else
      pComm->OnConnect();
}

acs_result ACustomSocket::ClassifyError(int error)
{
   acs_result res;
   switch (error) {
   case WSAENETDOWN:
   case WSAENETUNREACH:
      res = acsNetdown; // runtime: net is down
      break;
   case WSAEWOULDBLOCK:
      res = acsExpected; // runtime: operation continues, special case
      break;
   case WSAETIMEDOUT:
   case WSAECONNREFUSED:
   case WSAEHOSTDOWN:
   case WSAEHOSTUNREACH:
   case WSAENETRESET:
   case WSAECONNABORTED:
   case WSAECONNRESET:
   //case WSAECONNABORTED:
      res = acsTerminate; // runtime: operation completed without success
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
   case WSASYSNOTREADY:
   case WSAEINVAL: // Create() error in args (address "" must be NULL)
   case WSAEALREADY:
   case WSAENOTSOCK:
   case WSAEMSGSIZE:
   case WSAEADDRINUSE:
   case WSAEADDRNOTAVAIL:
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
   // stolen from article here: https://stackoverflow.com/questions/455434/how-should-i-use-formatmessage-properly-in-c
   CString result;
   TCHAR *err;
   DWORD errCode = nErrorCode;
   if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
      NULL,
      errCode,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
      (LPTSTR) &err,
      0,
      NULL))
      return "";

   //TRACE("ERROR: %s: %s", msg, err);
   //TCHAR buffer[1024];
   //_sntprintf_s(buffer, sizeof(buffer), _T("ERROR: %s: %s\n"), msg, err);
   result = err;
   LocalFree(err);
   return result;
}

CString ACustomSocket::getLastErrorString() const
{
   return this->GetWindowsErrorString(this->wsaError);
}
