#include "stdafx.h"

#include "PubBufferTransfer.h"

#include <algorithm>

extern "C" {
#include "pbbase64.h"
}

static const int MAX_MESSAGE = 32500; // some overhead included in the 32K Pubnub message max size

PNBufferTransfer::PNBufferTransfer()
   : buffer(nullptr)
{

}

PNBufferTransfer::~PNBufferTransfer()
{
   delete [] buffer;
}

void PNBufferTransfer::reserve(size_t cap)
{
   try {
      delete [] buffer;
      numUsed = 0;
      bufferCapacity = 0;
      buffer = new BYTE[cap];
      numUsed = 0;
      bufferCapacity = cap;
   }
   catch (std::exception& e) {
      TRACE("PNBT memory alloc failure for %u bytes:%s\n", cap, e.what());
      return;
   }
}

void PNBufferTransfer::resize(size_t newSize)
{
   // NOTE: assumes that capacity() is big enough, will not work if not so
   if (newSize <= this->capacity())
   {
      this->numUsed = (newSize);
      //TRACE("PNBT cap=%uB, was %uB + %uB = %uB\n", buffer.capacity(), currentSize, countBytes, newSize );
   }
}

bool PNBufferTransfer::initForCoding(size_t cap)
{
   this->reserve(cap);
   return true;
}

bool PNBufferTransfer::initForDecoding(size_t num_blocks, size_t block_size)
{
   //block_size = MAX_MESSAGE;
   size_t cap = num_blocks * block_size;
   // actually we need to use the smaller decoded length here
   //cap = pbbase64_decoded_length(cap);
   //cap += 10; // some extra bytes "just in case" - figure out if more is needed
   return this->initForCoding(cap);
}

void PNBufferTransfer::addBytes(const BYTE* pData, size_t countBytes)
{
   // could this be interrupted? need a crit-section?
   BYTE* pDest = this->end();
   memcpy( pDest, pData, countBytes );
   this->resize(countBytes + this->size());
}

size_t PNBufferTransfer::split_buffer(size_t section_size)
{
   if (section_size <= 0)
      return 0; // and no change to the previous contents of chunks

   chunks.clear();

   const size_t bsize = this->size();
   const size_t chksize = min(section_size, MAX_MESSAGE);
   const size_t num_full_chunks = bsize / chksize;
   const size_t last_chunk_size = bsize % chksize;
   //const size_t num_last_chunks = (last_chunk_size != 0);
   //TRACE("PNBT:encode will process %uB as %u chunks of %uB each and %d chunks of %uB\n",
   //   bsize, num_full_chunks, chksize, num_last_chunks, last_chunk_size);

   const BYTE* const pBStart = this->begin();
   const BYTE* const pBEnd = this->end();
   const BYTE* pB = pBStart; 
   size_t bufinc = chksize; //num_full_chunks ? chksize : last_chunk_size;
   for (size_t i = 0; pB < pBEnd;  pB += bufinc, ++i)
   {
      // adjust the loop increment for last time around if needed
      if (i == num_full_chunks) {
         bufinc = last_chunk_size;
      }
      //TRACE("..PBNT:Encoding bytes from [%u] to [%u]\n", pB-pBStart, pB+bufinc-pBStart);
      chunks.push_back(pB);
  }
   chunks.push_back(pBEnd);
   return chunks.size()-1; // don't count the end ptr, it's just there to make things easier for coded string construction
}

std::string PNBufferTransfer::getBufferSubstring(size_t n)
{
   std::string result;
   // make sure n is in range first [0..sz)
   if (n < chunks.size() - 1)
   {
      // and now do that encoding
      const BYTE* pStart = chunks[n];
      const BYTE* pEnd = chunks[n+1];
      const size_t nBytes = pEnd - pStart;
      const size_t ENC_BUFSZ = pbbase64_char_array_size_for_encoding(nBytes);
      static char buf[MAX_MESSAGE]; // don't frag the heap, just use one maximally sized buffer
      size_t nn = ENC_BUFSZ;
      const pubnub_bymebl_t to_encode = { (uint8_t*)pStart, nBytes };
      if (0 == pbbase64_encode_std(to_encode, buf, &nn)) {
         //then make and save a string out of the results
         result = buf;
      } else {
         // encoding error?
      }
   }
    return result;
}

void PNBufferTransfer::addCodedString( const std::string& chunk )
{
   if (!chunk.empty())
   {
      // decode the contents of the string into the next chunk of buffer space
      size_t countBytes = chunk.size();
      size_t countBytesAdjusted = pbbase64_decoded_length(countBytes);
      pubnub_bymebl_t to_decode = { (uint8_t*)this->end(), countBytesAdjusted };
      if (0 == pbbase64_decode_std(chunk.c_str(), countBytes, &to_decode)) {
         // this is successful, bytes are in place: save the start buffer pointer
         chunks.push_back(this->begin());
         // actual number of bytes written is updated in to_decode
         // make the new buffer size reflect what was added
         this->resize(to_decode.size + this->size());
      } else {
         // decoding error; with no size adjust, the next call will overwrite any used bytes
         // what kind of errors happen here?
      }
   }
}

bool PNBufferTransfer::operator==( const PNBufferTransfer& other ) const
{
   //if (this->buffer != other.buffer) return false;
   //if (this->capacity() != other.capacity())
   //   return false;
   if (this->size() != other.size())
      return false;
   for (size_t i = 0; i != this->size(); ++i)
      if (this->buffer[i] != other.buffer[i])
         return false;
   //if (this->chunks != other.chunks) return false;
   return true;
}

/*static*/ bool PNBufferTransfer::UnitTest()
{
   PNBufferTransfer test;
   const char* source = "012345678901234567890123456789"; // 30 chars
   size_t sz = strlen(source);

   // allocation size tests
   size_t cap = sz * 4 / 3; // TBD - use pubnub func.here
   test.initForCoding( cap );
   if (test.capacity() != cap)
      return false;

   // test the input step (raw bytes)
   const BYTE* s = reinterpret_cast<const BYTE*>(source);
   const int CHUNK_SIZE = 10;
   for (size_t i =0; i != cap - CHUNK_SIZE; i += CHUNK_SIZE) {
      test.addBytes( s + i, CHUNK_SIZE );
      if (test.size() != i + CHUNK_SIZE)
         return false;
   }

   // test the buffer splitter
   int sizes[] = { 5, 1, 2, 7, 25, 29, 30, 31, 0, -13 };
   for (int j = 0; j != sizeof sizes/sizeof sizes[0]; ++j) 
   {
      int SPLIT_SIZE = sizes[j];
      size_t num = test.split_buffer(SPLIT_SIZE);
      if (SPLIT_SIZE == 0) {
         if (num != 0) 
            return false;
      }
      else if (SPLIT_SIZE < 0) {
         if (num != 1) // acts like a HUGE number (unsigned), so one chunk ptr and the end ptr
            return false;
      }
      else if (num != (sz / SPLIT_SIZE + ((sz % SPLIT_SIZE) ? 1 : 0)))
         return false;
   }

   // test overall encoding process with decoding (test passes if buffers the same)
   int SPLIT_SIZE = sizes[0];
   int x = test.split_buffer(SPLIT_SIZE);
   PNBufferTransfer test2;
   test2.initForDecoding(x, SPLIT_SIZE);
   for (int k = 0; k < x; ++k) {
      std::string s = test.getBufferSubstring(k);
      test2.addCodedString(s);
   }
   if (!(test == test2))
      return false;

   return true;
}
