#pragma once


enum ConnectionType { 
   kNotSet, 
   kPrimary, 
   kSecondary, 
   kNoChange, 
};

enum ConnectionStatus {
   kDisconnected, // no local link
   //kConnecting, // transient
   kConnected, // no remote link
   //kRemoting, // transient
   kChatting, // both links, no command modes
   kScrolling, // CMD: scrolling is in progress
   kFileSending, // CMD: file send is in progress
   kFileRcving, // CMD: file rcv is in progress
};
