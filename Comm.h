// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include <vector>

//-----------------------------------------------------------------------
// includes

//-----------------------------------------------------------------------
// classes

class ACommImpl;

class AComm
{
public:
   // commands
   enum Command {
      kScroll           = 'g',
      kTimer            = 't',
      kScrollSpeed      = 's',
      kScrollPos        = 'p',
      kScrollPosSync    = 'y',
      kScrollMargins    = 'm',
      kCueMarker        = 'c',
      kFrameInterval    = 'f',

      kContactRemote    = 'C',

      // modifiers
      kOn            = 2,
      kOff           = 3,
      kReset         = 4,
      kPrev          = 5,
      kNext          = 6
   };

   enum State {
      kUnknown,
      kIdle, 
      kConnecting, 
      kDisconnecting, 
      kConnected, 
      kChatting
   };

   enum OpType {
      kNone,
      kLogin,
      kLogout,
      kConnect,
      kDisconnect,
      kScrollOn,
      kScrollOff,
      kFileSend,
      kFileReceive,
      kFileCancel,
      kContact,
      kContactCancel,
   };
   
   enum Status {
      kSuccess,
      kFailure,
      kWaiting = 50,
      kGenericErrorStatus = 100,
      kUnconfigured,
      kNoChannelName,
      kUnableToLogin,
      kUnableToLogout,
      kUnableToPair,
      kUnableToUnpair,
      kUnableToSendCommand,
      kUnableToGetResponse,
      kAlreadyInProgress,
      kUnableToStartScroll,
      kUnableToStopScroll,
      kUnableToContact,
      kContactRejected,
      kBadFormat,
      kOutsideDomain,
   };

   class ASlave
   {
   public:
      CString fInstallation;
      CString fUsername;
   };
   typedef std::vector<ASlave> Slaves;

   AComm();
   virtual ~AComm();

   /** return true if an internet connection is detected
     */
   bool IsOnline() const;

   /** establish a connection using the specified JID information.
      Re-connect any time the name or pwd changes. If a connection 
      already exists, it will be dropped and a new one established.
     */
   void Connect(LPCTSTR name, LPCTSTR pwd);

   /** attempt to start a chat session with the specified target.
      Returns true if the chat was successfully started.
     */
   bool StartChat(LPCTSTR target);

   /** stop the current chat session.
     */
   void EndChat();

   /** begin a file transfer
     */
   bool SendFile(LPCTSTR file);

   /** terminate the current connection and session and go to idle
     */
   void Disconnect();

   /** enter a new state. Parent will be notified in some circumstances.
     */
   void SetState(State newState);

   /** set the handle of the window that will receive status
      notifications.
     */
   void SetParent(HWND parent);
   HWND GetParent() const;

   /** send a command to the connected slave instance
      return false is sending the command failed
     */
   bool SendCommand(Command cmd, int param1, int param2=0);
   bool SendCommand(Command cmd, const std::string& param=std::string());

   /** serialization of settings, slaves, etc.
     */
   bool Read();
   bool Write();
   bool ConfigureSettings();

   CString GetUsername() const;
   void SetUsername(LPCTSTR username);
   CString GetPassword() const;
   void SetPassword(LPCTSTR password);
   CString GetSessionJid() const;
   Slaves& GetSlaves();
   bool IsMaster() const;
   bool IsSlave() const;
   bool IsConnected() const;
   CString GetLastMessage() const;
   bool IsContacted() const;
   void ResendLastStatus() const;

   // for use by remote comm implementation, when a transaction completes an operation with a status code
   void OnStateChange(OpType operation, Status statusCode);
   void RemoteBusyWait();

private:
   static UINT Connect(LPVOID param);

   ACommImpl* fImpl;
   State fState;
   CString fUsername;
   CString fPassword;
   std::vector<ASlave> fSlaves;
   bool fIsMaster;
   CWinThread* fThread;
};

//-----------------------------------------------------------------------------
// inline functions
inline CString AComm::GetUsername() const
{
   return fUsername;
}

inline void AComm::SetUsername(LPCTSTR username)
{
   ASSERT(NULL != username);
   fUsername = username;
}

inline CString AComm::GetPassword() const
{
   return fPassword;
}

inline void AComm::SetPassword(LPCTSTR password)
{
   ASSERT(NULL != password);
   fPassword = password;
}

inline AComm::Slaves& AComm::GetSlaves()
{
   return fSlaves;
}

inline bool AComm::IsMaster() const
{
   return fIsMaster;
}

inline bool AComm::IsSlave() const
{
   return this->IsConnected() && !this->IsMaster();
}

inline bool AComm::IsConnected() const
{
   return (AComm::kConnected == fState) || (AComm::kChatting == fState);
}

inline void AComm::SetState(State newState)
{
   if (newState != fState) {
      fState = newState;
   }
}

