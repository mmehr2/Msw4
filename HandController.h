// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

class AHandController
{
public:
   enum {
      kRightButton      = 1,
      kLeftButton       = 2,
      kPortClosed       = -1,
      kMaxPorts         = 16
   };

   AHandController() {this->Init(); this->Connect();}
   ~AHandController() {this->Disconnect();}

   bool IsConnected() const;

  // returns TRUE if controller synced
   bool Poll(int& speed, bool& lClick, bool& rClick);

   void Connect();
   void Disconnect();

private:
   // methods
   void Init() {fPortId = INVALID_HANDLE_VALUE; fSwitches = 0;}

   // data
   static const BYTE kSyncChar   = 'M';
   enum {
      kBaudRate      = 1200,
      kInQueueSize   = 512,
      kOutQueueSize  = 128,
   };

   HANDLE fPortId;
   int fSwitches;
};

extern AHandController handControl;  // global object

///////////////////////////////////////////////////////////////////////////////
// inline functions
inline bool AHandController::IsConnected() const
{
   return (INVALID_HANDLE_VALUE != fPortId);
}
