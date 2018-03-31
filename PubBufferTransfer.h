#pragma once

#include <vector>
#include <string>

#include <wtypes.h>

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
Typical Sender usage:
   1. call initForCoding(), passing the total number of bytes expected; this resets the object for use as sender
   2. call addBytes() repeatedly, adjusting the source pointer and count externally (caller responsibility)
   3. when buffer is loaded, call split_buffer(), specifying the chunk size to create, and remembering the chunk count N
   4. then it's okay to call getBufferSubstring(n), which will return a coded string for chunk n (in any order, 0<=n<N)
Typical Receiver usage:
   1. call initForDecoding(), passing the number of coded string blocks expected, as well as the encoded block size;
      this resets the object for use as a receiver
   2. call addCodedString() as each string chunk arrives (this will decode and add to the buffer)
   3. call startRead() to reset the internal iterator and return the total byte count (if needed) - can be repeated
   4. call getBytes() repeatedly, providing the destination pointer and byte count desired for each call
*/

class PNBufferTransfer {
   typedef BYTE* buffer_type;
   typedef std::vector<const BYTE*> buffer_ptr_array;
   typedef const BYTE* buffer_const_iterator;

   buffer_type buffer;
   size_t numUsed; // maintains "size" position to last used byte +1 (next open pos)
   size_t bufferCapacity; // allocated max size
   buffer_const_iterator read_it;

   buffer_ptr_array chunks;
   
   // helper
   void resize( size_t num_bytes );
   void reserve( size_t num_bytes );
   BYTE* bufferStart();
   const BYTE* bufferStart() const;
   // buffer std::vector simulation routines
   size_t size() const { return numUsed; }
   size_t capacity() const { return bufferCapacity; }
   BYTE* begin() { return bufferStart(); }
   BYTE* end() { return begin() + size(); }
   const BYTE* begin() const { return bufferStart(); }
   const BYTE* end() const { return begin() + size(); }
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
   size_t addBytes(const BYTE* pSource, size_t countBytes);
   // receiver - adds the next block to the internal buffer, decoding in the process
   void addCodedString( const std::string& chunk );

   // step 3 - incrementally produce or consume transfer blocks
   // sender - needs block count up front; it gets this from splitting the buffer
   size_t split_buffer(size_t section_size); // returns # of chunks generated
   // receiver - start the read operation that getBytes() will use; returns size() of buffer
   size_t startRead();

   // step 4 - extract results when done
   // sender - gets the Nth block as a string to add to the publishing queue (PQ)
   // the plan is to do the individual encoding here, since this will be called on the sub callback thread
   // NOTE: DO NOT ADD BYTES IN BETWEEN CALLS TO THIS FUNCTION
   std::string getBufferSubstring(size_t n);
   // receiver - get the raw bytes incrementally (can be called from RichEdit control EDITSTREAM callback)
   // params:
   //    pDestination - where to put the data
   //    countBytes - how many bytes to transfer out during this read
   // returns: actual number of bytes transferred
   // NOTE: must call startRead() to position the reader pointer at buffer start before calling this
   //    Count is adjusted internally to copy only as many bytes as are in the buffer currently; could be 0
   size_t getBytes(BYTE* pDestination, size_t countBytes);

   friend static bool UnitTestOnce(const char*, size_t);
   static bool UnitTestOnce(const char* pSource, size_t maxBlockSize);
   static bool UnitTest();

   // useful for tests
   bool operator==( const PNBufferTransfer& other ) const; // comparing just buffers for now

};
