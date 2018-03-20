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
   //TRACE("PNBT:encode will process %uB as %u chunks of %uB each and %d chunks of %uB\n",
   //   bsize, num_full_chunks, chksize, num_last_chunks, last_chunk_size);

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
      //TRACE("..PBNT:Encoding bytes from [%u] to [%u]\n", pB-pBStart, pB+bufinc-pBStart);
      chunks.push_back(pB);
  }
   chunks.push_back(pBEnd);
   return chunks.size()-1; // don't count the end ptr, it's just there to make things easier
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
      const pubnub_bymebl_t to_encode = { (uint8_t*)pStart, nBytes - 1 };
      if (0 == pbbase64_encode_std(to_encode, buf, &nn)) {
         //then make and save a string out of the results
         result = buf;
      }
   }
    return result;
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

   int x = test.split_buffer(sizes[0]);
   std::vector<std::string> queue;
   for (int k = 0; k < x; ++k) {
      std::string s = test.getBufferSubstring(k);
      queue.push_back(s);
   }

   return true;
}
