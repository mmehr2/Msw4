#pragma once


enum ConnectionType { 
   kNotSet, 
   kPrimary, 
   kSecondary, 
   kNoChange, 
};

enum ConnectionStatus {
   kDisconnected,
   kConnecting,
   kConnected,
};
