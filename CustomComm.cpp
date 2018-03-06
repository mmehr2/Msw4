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
   , pSocket(NULL)
   , pListenerSocket(NULL)
   , protocol_port(PROTOCOL_PORT)
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

void ACustomComm::SetParent(HWND parent)
{
   // SECONDARY: will forward all incoming messages to this window, after receipt of incoming data from Service
   fParent = parent;
}

bool ACustomComm::Initialize(ConnectionType conn_typ)
{
   // NoChange keeps old setting; NotSet should not be used here but causes no harm
   if (conn_typ != kNoChange && conn_typ != kNotSet) {
      fConnection = conn_typ;
   }

   // initialize the socket libraries here (called at startup currently)
   static bool any_init = false;
   if (!any_init) {
      ::AfxSocketInit();
      TRACE("SET UP SOCKET SYSTEM.\n");
      any_init = true;
   }

   TRACE("SET UP LINK TO REMOTE... AS %s.\n", this->isPrimary() ? "PRIMARY" : this->isSecondary() ? "SECONDARY" : "NOT SET");

   return true;
}

void ACustomComm::Deinitialize()
{
   this->CloseLink(); // if anything in progress
   //fConnection = kNotSet; // actually don't forget our last choice, don't use this
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

         // call the Configure(port, address) command here to actually create the socket properly for MFC Sockets
         if (this->isPrimary()) {
            pListenerSocket = NULL;
            pSocket = pOurSocket;
            // PRIMARY: set up the communications socket
            typeStr.Format(_T("PRIMARY"));
            operation.Format(_T("ConfigureX(port=%d,addr=%s)"), protocol_port, address);
            pOurSocket->Configure(protocol_port, std::string(address));
            if (!pOurSocket->isOK()) { goto Errors; }
            // then establish the connection
            operation.Format(_T("ConnectX(port=%d,addr=%s)"), protocol_port, address);
            pOurSocket->ConnectX();
            if (!pOurSocket->isOK()) { goto Errors; }
            fLinked = kConnected;
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
            fLinked = kConnected;
         }
         else {
            fLinked = kDisconnected; // not decided what type? just idle it - or delete??
         }
         goto NoErrors;
Errors:
         CString err, classErr = _T("Generic error.");
         if(pOurSocket->isProgrammerError()) 
            classErr.Format(_T("Programming error."));
         if(pOurSocket->isDown()) 
            classErr.Format(_T("Network subsystem failed."));
         if(pOurSocket->isResourceError()) 
            classErr.Format(_T("Ran out of runtime resources."));
         err.Format(_T("%S START LINK ERROR = %S ATTEMPTING TO %S"), typeStr, classErr, operation);
         throw(std::exception(CT2A(err)));
NoErrors:
         err.Format(_T("%S START LINK SUCCESS ATTEMPTING TO %S.\n"), typeStr, operation);
         TRACE(err);
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
