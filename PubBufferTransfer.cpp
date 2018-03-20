#include "stdafx.h"

#include "PubBufferTransfer.h"

#include <algorithm>

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
      TRACE("PNBT cap=%u B, was %u B + %u B = %u B\n", buffer.capacity(), currentSize, countBytes, newSize );
   }
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

   return true;
}
