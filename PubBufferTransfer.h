#pragma once

#include <vector>
#include <string>

#include <afx.h>

/*
File Encoder/Decoder Object

The process of transfer, starting on the Sender end (PRIMARY), it will:
   take bytes from the RichEdit control ,
   split them into proper sized chunks for sending, limited by 32K- Pubnub message size,
   kick off the transfer with a command including size of transmission (# of blocks),
   then on the callback thread from the publish-OK, get the next string (encodes as it extracts)
Then on the Receiving end (SECONDARY) it will:
   interpret the number of blocks (by convention? send size too?) to set up the buffer capacity,
   add each block as it is received, decode it, and add the bytes to the internal buffer,
   in parallel, assemble the stack of chunk pointers (not needed?)
   notify the UI code to transfer the new bytes into the RichEdit control
*/

class PNBufferTransfer {
   typedef BYTE* buffer_type;
   typedef std::vector<const BYTE*> buffer_ptr_array;

   buffer_type buffer;
   size_t numUsed; // maintains "size" position to last used byte +1 (next open pos)
   size_t bufferCapacity; // allocated max size

   buffer_ptr_array chunks;
   
   // helper
   void resize( size_t num_bytes );
   void reserve( size_t num_bytes );
   // buffer std::vector simulation routines
   BYTE* begin() { return buffer; }
   BYTE* end() { return buffer + numUsed; }
   const BYTE* begin() const { return buffer; }
   const BYTE* end() const { return buffer + numUsed; }
   size_t size() const { return numUsed; }
   size_t capacity() const { return bufferCapacity; }
public:
   PNBufferTransfer();
   ~PNBufferTransfer();

   // step 1 (send OR receive) - set up the buffer capacity
   // sender - gets this from the size of the contents of the RichEdit control
   bool initForCoding(size_t cap);
   // receiver - gets this from the file transfer start (F) command
   bool initForDecoding(size_t num_blocks, size_t block_size);

   // Step 2 load the input; sender gets from RichEdit control, receiver gets from subscribe callback
   // sender - uses this function that can be called from RichEdit control EDITSTREAM callbacks
   void addBytes(const BYTE* pData, size_t countBytes);
   // receiver - adds the next block to the internal buffer, decoding in the process
   void addCodedString( const std::string& chunk );

   // step 3 - incrementally produce or consume transfer blocks
   // sender - needs block count up front; it gets this from splitting the buffer
   size_t split_buffer(size_t section_size); // returns # of chunks generated
   // receiver - creates internal buffer of decoded bytes from input strings
   void decode(); // actually, I don't believe there is a need for this, unless it's an overall wrapup

   // step 4 - extract results when done
   // sender - gets the Nth block as a string to add to the publishing queue (PQ)
   // the plan is to do the individual encoding here, since this will be called on the sub callback thread
   std::string getBufferSubstring(size_t n);
   // receiver - get the raw bytes incrementally (can be called from RichEdit control EDITSTREAM callback)
   void getBytes(const BYTE* pData, size_t countBytes);

   friend static bool UnitTest();
   static bool UnitTest();

   // useful for tests
   bool operator==( const PNBufferTransfer& other ) const; // comparing just buffers for now

};
