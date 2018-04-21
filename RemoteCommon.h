#pragma once

#include <string>

enum ConnectionType { 
   kNotSet, 
   kPrimary, 
   kSecondary, 
   kNoChange, 
};

enum ConnectionStatus {
   kDisconnected, // not online (logged out)
   kConnecting, // transient v
   kDisconnecting, // transient ^
   kConnected, // online, has contact with system but not in paired communications
   kLinking, // transient v
   kUnlinking, // transient ^
   kChatting, // paired for comm, but no commands in progress
   kBusy, // transaction in progress
   kScrolling, // CMD: scrolling is in progress
   kFileSending, // CMD: file send is in progress
   kFileReceiving, // CMD: file rcv is in progress
   kFileCanceling, // CMD: file cancel is in progress
};

namespace remchannel {

   enum type {
      kReceiver,
      kSender,
   };

   enum state {
      kNone, // starts here, needs activation info
      kDisconnected, // no active connection
      kConnecting, // in process of setting up connection
      kDisconnecting, // in process of shutting down connection
      kIdle, // connected, no active transaction or command
      kBusy, // transaction in process (sub, pub, or time), waiting for callback
   };

   enum result {
      kOK,
      kError,
   };

   enum JSON {
      PREFIX = '\"',
      SUFFIX = '\"'
   };

   extern const std::string JSON_PREFIX, JSON_SUFFIX;
   extern const char* GetTypeName( type t );
   extern const char* GetStateName( state s );
}

namespace rem {

   extern const char* GetConnectionTypeName( ConnectionType t );
   extern const char* GetConnectionStateName( ConnectionStatus s );

}

