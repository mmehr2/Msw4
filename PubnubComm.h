#pragma once

#include <afx.h>

class AComm;

class APubnubComm
{
   AComm* fComm;
   HWND fParent;
   bool fPrimary;
public:
   APubnubComm(AComm* pComm);
   ~APubnubComm(void);

   /** set the handle of the window that will receive status
      notifications.
     */
   void SetParent(HWND parent);

   // Will set up the connection to either publish or subscribe
   // returns false if immediate errors
   bool Connect(bool as_primary);

   // PRIMARY: called to send a message via the interface to the SECONDARY
   void SendMessage(const char* message);

   // SECONDARY: called to dispatch any received message from the interface to the parent window
   void OnMessage(const char* message);

};

