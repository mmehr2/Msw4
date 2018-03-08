#include "stdafx.h"
#include "CustomComm.h"
#include "CustomSocket.h"

#include "Comm.h"

#include <iostream>


// THIS VERSION WILL BE BASED ON CAsyncSocket()



ACustomComm::ACustomComm(AComm* pComm) 
   : fComm(pComm)
   , fParent(NULL)
   , fConnection(kNotSet)
   , fLinked(kDisconnected)
   , pSocket(NULL)
   , pListenerSocket(NULL)
   , protocol_port(PROTOCOL_PORT)
   , local_address("")
   , last_sockerror(0)
{
}


ACustomComm::~ACustomComm(void)
{
   this->CloseLink();

   // no ownership of parent or comm layer
   fParent = NULL;
   fComm = NULL;
   fConnection = kNotSet;
}

CString ACustomComm::getLastCommErrorString() const
{
   return ACustomSocket::GetWindowsErrorString(this->last_sockerror);
}

int ACustomComm::getLastCommErrorCode() const
{
   return (this->last_sockerror);
}

void ACustomComm::SetParent(HWND parent)
{
   // SECONDARY: will forward all incoming messages to this window, after receipt of incoming data from Service
   fParent = parent;
}

bool ACustomComm::Initialize(ConnectionType conn_typ, const char* local_IP, int port)
{
   // NoChange keeps old setting; NotSet should not be used here but causes no harm
   if (conn_typ != kNoChange && conn_typ != kNotSet) {
      fConnection = conn_typ;
      // if local IP is provided, configure it here
      if (local_IP != NULL) {
         local_address = local_IP;
         TRACE("NEW LOCAL IP ADDRESS PROVIDED/SAVED: %s\n", local_IP);
      }
      // if local port is provided, configure it here (ignore if <0)
      if (port >= 0) {
         if (!port)
            port = PROTOCOL_PORT; // default if set to 0
         if (protocol_port != port)
            TRACE("NEW LOCAL PORT PROVIDED/SAVED: %d\n", port);
         protocol_port = port;
      }
   }

   // initialize the socket libraries here (called at startup currently)
   static bool any_init = false;
   if (!any_init) {
      ::AfxSocketInit();
      TRACE("SET UP SOCKET SYSTEM.\n");
      any_init = true;
   }

   //TRACE("SET UP LINK TO REMOTE... AS %s.\n", local_address.c_str());
   TRACE(("CONFIGURING REMOTE COMM PROTOCOL AS %s, LOCAL IP = %s"),
      this->isPrimary() ? ("PRIMARY") : this->isSecondary() ? ("SECONDARY") : ("NOT SET"),
      local_address.c_str()
      );
   //TRACE(_T("CONFIGURING REMOTE COMM PROTOCOL AS %s, LOCAL IP = %s"),
   //   this->isPrimary() ? _T("PRIMARY") : this->isSecondary() ? _T("SECONDARY") : _T("NOT SET"),
   //   (LPCTSTR)CString(local_address.c_str())
   //   );

   return true;
}

void ACustomComm::Deinitialize()
{
   this->CloseLink(); // if anything in progress
   //fConnection = kNotSet; // actually don't forget our last choice, don't use this
}

CString TraceHelper(const ACustomSocket* pOurSocket, const CString& typeStr, const CString& operation)
{
   CString err, classErr = _T("Generic error.");
   if(pOurSocket->isProgrammerError()) 
      classErr.Format(_T("Programming error."));
   if(pOurSocket->isDown()) 
      classErr.Format(_T("Network subsystem failed."));
   if(pOurSocket->isResourceError()) 
      classErr.Format(_T("Ran out of runtime resources."));
   err.Format(_T("%s START LINK ERROR = %s ATTEMPTING TO %s\n%s\n"), 
      (LPCTSTR)typeStr, (LPCTSTR)classErr, (LPCTSTR)operation, (LPCTSTR)pOurSocket->getLastErrorString());
   return err;
}

bool ACustomComm::OpenLink(const char * address)
{
   bool result = true;
   try {
      if (!this->isSet()) {
         throw(std::exception("ERROR: Unable to establish link of unknown type."));
      }
      ACustomSocket* pOurSocket;
      CString operation, typeStr;
      pOurSocket = new ACustomSocket(this); // secondary only; primary uses a listener until connection arrives, then makes this the connected socket
      if (NULL == pOurSocket) {
         // out of memory? total failure?
         throw(std::exception("Failed to create the socket."));
      } else {
         fLinked = kConnecting; // restart the link process
         last_sockerror = 0;

         // call the Configure(port, address) command here to actually create the socket properly for MFC Sockets
         if (this->isPrimary()) {
            pListenerSocket = NULL;
            pSocket = pOurSocket;
            // PRIMARY: set up the communications socket, using the local_address
            typeStr.Format(_T("PRIMARY"));
            if (!pOurSocket->isOK()) { goto Errors; }
            LPCTSTR addr_ = (LPCTSTR)CString( CA2T(address) );
            operation.Format(_T("ConfigureX(port=%d,addr=%s)"), protocol_port, addr_);
            pOurSocket->Configure(protocol_port); // local address is already known
            if (!pOurSocket->isOK()) { goto Errors; }
            // then establish the connection
            operation.Format(_T("ConnectX(port=%d,addr=%s)"), protocol_port, addr_);
            pOurSocket->ConnectX(address);
            // handle WouldBlock by calling OnCOnnect only if no errors
            if (pOurSocket->isOK()) {
               // IMMEDIATE COMPLETION (localhost?)
               OnConnect();
            }
            else if (pOurSocket->isContinuing()) {
               // WouldBlock here means we have to wait for the results asynchronously, socket will call OnConnect() later
               TRACE(_T("CONNECTION IN PROGRESS WAITING FOR RESULTS\n"));
               result = true; // but doesn't tell the full story ...
               goto Done;
            }
            else { goto Errors; }
         } 
         else if (this->isSecondary()) {
            pListenerSocket = pOurSocket;
            pSocket = NULL;
            // SECONDARY: set up the Listener socket (default address means just listen for any address on the port)
            typeStr.Format(_T("SECONDARY"));
            operation.Format(_T("ConfigureX(port=%d,addr=ANY)"), protocol_port);
            pOurSocket->Configure(protocol_port);
            if (!pOurSocket->isOK()) { goto Errors; }
            // start listening for connection requests
            operation.Format(_T("ListenX(port=%d)"), protocol_port);
            pOurSocket->ListenX();
            if (!pOurSocket->isOK()) { goto Errors; }
         }
         else {
            fLinked = kDisconnected; // not decided what type? just idle it - or delete??
         }
         goto NoErrors;
Errors:
         throw(std::exception(CT2A(TraceHelper(pOurSocket, typeStr, operation))));
NoErrors:
         TRACE(_T("%s START LINK SUCCESS ATTEMPTING TO %s.\n"), (LPCTSTR)typeStr, (LPCTSTR)operation);
Done:
         ; // to avoid syntax errors - sorry about the goto's!
      }
   } catch (std::exception x) {
      TRACE("DEMO SOCKET ERROR: %s", x.what());
      AfxMessageBox(CA2W(x.what()));
      result = false;
   }

   return result; // started sequence successfully
}

bool ACustomComm::CloseLink()
{
   if (!this->isDisconnected()) {
      // disconnect any link already set up or in progress
      TRACE("DISCONNECTING LINK");
      if (pSocket) {
         pSocket->Close();
         TRACE(" OF PRIMARY COMM SOCKET\n");
      }
      if (pListenerSocket) {
         pListenerSocket->Close();
        TRACE(" OF SECONDARY LISTENER SOCKET\n");
      }
      // remove the objects, free memory
      delete pSocket;
      pSocket = NULL;
      delete pListenerSocket;
      pListenerSocket = NULL;
   }
   fLinked = kDisconnected; // restart the link process
   return true;
}

void ACustomComm::OnAccept() {
   int error = pSocket->getLastErrorCode();
   CString errstr = pSocket->getLastErrorString();
   if (!pSocket->isOK()) {
      TRACE(_T("COMPLETED SOCKET ACCEPT, ERROR %d: %s\n"), error, (LPCTSTR)errstr);
      return;
   }
   TRACE("RECEIVED SOCKET ACCEPT REQUEST.\n");
    // TBD we need to create the comm link
}

void ACustomComm::OnConnect() {
   int error = pSocket->getLastErrorCode();
   CString errstr = pSocket->getLastErrorString();
   if (!pSocket->isOK()) {
      TRACE(_T("COMPLETED SOCKET CONNECT, ERROR %d: %s\n"), error, (LPCTSTR)errstr);
      return;
   }
   TRACE("COMPLETED SOCKET CONNECT REQUEST.\n");
   // TBD: this code must be conditional on errors reported; call it here only if we have no errors on the Connect (WouldBlock is normal here)
   fLinked = kConnected;
   UINT local_port = protocol_port;
   CString local_adr;
   int res = pSocket->GetSockName( local_adr, local_port );
   pSocket->PostProcessError(res);
   CString operation, typeStr;
   operation.Format(_T("Get Name test = (port=%d,addr=%s)\n"), local_port, (LPCTSTR)CString(local_adr));
}
