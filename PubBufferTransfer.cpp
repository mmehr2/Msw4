#include "stdafx.h"

#include "PubBufferTransfer.h"

#include <algorithm>

extern "C" {
#include "pbbase64.h"
}

static const int MAX_MESSAGE = 32500; // some overhead included in the 32K Pubnub message max size

PNBufferTransfer::PNBufferTransfer()
{

}

PNBufferTransfer::~PNBufferTransfer()
{

}

bool PNBufferTransfer::initForCoding(size_t cap)
{
   buffer.clear();
   buffer.reserve(cap);
   return true;
}

void PNBufferTransfer::addBytes(const BYTE* pData, size_t countBytes)
{
   // NOTE: assumes that capacity() is big enough
   size_t currentSize = buffer.size();
   size_t newSize = currentSize + countBytes;
   if (newSize < buffer.capacity())
   {
      buffer_type::iterator currentEnd = buffer.end();
      buffer.resize(newSize);
      memcpy( &(*currentEnd), pData, countBytes );
      //TRACE("PNBT cap=%uB, was %uB + %uB = %uB\n", buffer.capacity(), currentSize, countBytes, newSize );
   }
}

bool PNBufferTransfer::encodeBuffer()
{

   //const size_t ENC_LEN = pbbase64_encoded_length(chksize);
   //const size_t ENC_BUFSZ = pbbase64_char_array_size_for_encoding(chksize);
   //TRACE("PNBT:enc chunk encoded len=%uB, alloc=%uB\n", ENC_LEN, ENC_BUFSZ);

   return false;
}

size_t PNBufferTransfer::split_buffer(size_t section_size)
{
   bool result = true;
   if (section_size <= 0)
      return false;

   chunks.clear();

   const size_t bsize = buffer.size();
   const size_t chksize = min(section_size, MAX_MESSAGE);
   const size_t num_full_chunks = bsize / chksize;
   const size_t last_chunk_size = bsize % chksize;
   const size_t num_last_chunks = (last_chunk_size != 0);
   TRACE("PNBT:encode will process %uB as %u chunks of %uB each and %d chunks of %uB\n",
      bsize, num_full_chunks, chksize, num_last_chunks, last_chunk_size);

   const BYTE* const pBStart = &buffer[0];
   const BYTE* const pBEnd = pBStart + bsize;
   const BYTE* pB = pBStart; 
   size_t bufinc = chksize; //num_full_chunks ? chksize : last_chunk_size;
   for (size_t i = 0; pB < pBEnd;  pB += bufinc, ++i)
   {
      // adjust the loop increment for last time around if needed
      if (i == num_full_chunks) {
         bufinc = last_chunk_size;
      }
      TRACE("..PBNT:Encoding bytes from [%u] to [%u]\n", pB-pBStart, pB+bufinc-pBStart);
      chunks.push_back(pB);
      // and now do that encoding
      // then make and save a string out of the results
      // push the string back onto the array
   }
   chunks.push_back(pBEnd);
   return chunks.size();
}

/*static*/ bool PNBufferTransfer::UnitTest()
{
   PNBufferTransfer test;
   const char* source = "012345678901234567890123456789"; // 30 chars
   size_t sz = strlen(source);
   size_t cap = sz * 4 / 3; // use pubnub func.here
   test.initForCoding( cap );
   if (test.buffer.capacity() != cap)
      return false;

   const BYTE* s = reinterpret_cast<const BYTE*>(source);
   const int CHUNK_SIZE = 10;
   for (size_t i =0; i != cap - CHUNK_SIZE; i += CHUNK_SIZE) {
      test.addBytes( s + i, CHUNK_SIZE );
      if (test.buffer.size() != i + CHUNK_SIZE)
         return false;
   }

   int sizes[] = { 1, 2, 5, 7, 25, 29, 30, 31, 0, -13 };
   for (int j = 0; j != sizeof sizes/sizeof sizes[0]; ++j) 
   {
      int SPLIT_SIZE = sizes[j];
      size_t num = test.split_buffer(SPLIT_SIZE);
      if (SPLIT_SIZE == 0) {
         if (num != 0) 
            return false;
      }
      else if (SPLIT_SIZE < 0) {
         if (num != 2) // acts like a HUGE number (unsigned), so one chunk ptr and the end ptr
            return false;
      }
      else if (num != (sz / SPLIT_SIZE + 1 + ((sz % SPLIT_SIZE) ? 1 : 0)))
         return false;
   }

   return true;
}
