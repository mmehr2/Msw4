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
   // free any old buffer
   delete [] buffer;
   numUsed = 0;
   bufferCapacity = 0;
   buffer = nullptr;
   // 
   // we can probably do better than just fail if we run out of or fragment the heap
   // even one static buffer (size of one 32kB msg block) as fall-back would allow operation
   // This can operate in a loop, trying 1/2 the capacity each time until a) success or b) smaller than 32kb
   size_t tent_cap = cap, i=1;
   do// (size_t tent_cap = cap, i=1; tent_cap < MAX_MESSAGE; tent_cap /= 2, ++i)
   {
      try {
         buffer = new BYTE[cap];
         numUsed = 0;
         bufferCapacity = cap;
         TRACE("PNBT memory alloc OK %u bytes on try #%u\n", cap, i);
         break;
      }
      catch (std::exception& e) {
         TRACE("PNBT heap alloc attempt #%u failure for %u bytes:%s\n", i, cap, e.what());
      }
      tent_cap /= 2, ++i;
   } while (tent_cap > MAX_MESSAGE);
   if (buffer == nullptr) {   
      // this COULD happen - what to do?
      TRACE("PNBT heap alloc failure - using backup buffer.\n");
   }
}

static BYTE backupBuffer[MAX_MESSAGE * 10];

BYTE* PNBufferTransfer::bufferStart()
{
   return (buffer == nullptr) ? backupBuffer : buffer;
}

const BYTE* PNBufferTransfer::bufferStart() const
{
   return (buffer == nullptr) ? backupBuffer : buffer;
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

size_t PNBufferTransfer::addBytes(const BYTE* pData, size_t countBytes)
{
   size_t countTransferred = countBytes;
   if (pData && countBytes) {
      if (countBytes + numUsed > this->capacity())
         countTransferred = this->capacity() - this->numUsed;
      BYTE* pDest = this->end();
      memcpy( pDest, pData, countTransferred );
      this->resize(countTransferred + this->size());
   }
   return countTransferred;
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
   //chunks.push_back(pBEnd); // hard to maintain in incremental situations, just add it when splitting
   return chunks.size();
}

std::string PNBufferTransfer::getBufferSubstring(size_t n)
{
   std::string result;
   // make sure n is in range first [0..sz)
   if (n < chunks.size())
   {
      // special case for last chunk: add the missing end()
      bool is_last = n == chunks.size() - 1;
      // and now do that encoding
      const BYTE* pStart = chunks[n];
      const BYTE* pEnd = is_last ? this->end() : chunks[n+1];
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

size_t PNBufferTransfer::startRead()
{
   this->read_it = this->begin();
   return this->size();
}

size_t PNBufferTransfer::getBytes(BYTE* pData, size_t countBytes)
{
   size_t countTransferred = 0;
   if (pData && countBytes > 0) {
      // trim the transfer count according to what's still unread, if anything
      size_t bytesLeftToRead = this->end() - this->read_it;
      countTransferred = countBytes;
      if (countBytes > bytesLeftToRead) 
         countTransferred = bytesLeftToRead;
      // copy only if something left
      if (countTransferred > 0) {
         memcpy(pData, this->read_it, countTransferred);
         this->read_it += countTransferred;
      }
   }
   return countTransferred;
}

///////////////////// CLASS UNIT TESTING //////////////////////

/*static*/ bool PNBufferTransfer::UnitTest()
{
#ifdef _DEBUG
   const char* source = "0123456\r890123456789012\n456789"; // 30 chars w. embedded ctrl chars
   if (!UnitTestOnce(source, 5))
      return false;
   // big buffer test
   std::string s(source);
   // make it some number of times this big by replication
   int times = 1500;
   int doubles = int(log((double)times)/log(2.0) + 0.5);
   for (int i=0; i<doubles; ++i)
      s += s;
   if (!UnitTestOnce(s.c_str(), MAX_MESSAGE))
      return false;
#endif //def _DEBUG
   return true;
}

#include <ctime>
extern time_t get_local_timestamp();

class TestTimer {
   time_t tStart;
   std::string description;
   bool passed;
public:
   TestTimer(const char* descr)
   {
      description = descr;
      tStart = get_local_timestamp();
   }
   ~TestTimer()
   {
      time_t tEnd = get_local_timestamp();
      double td = difftime(tEnd, tStart) / 1.0e7;
      TRACE("%s timediff = %1.6lfs.\n", description.c_str(), td);
   }
   void addResult(bool pass, const std::string& moreDesc = "")
   {
      passed = pass;
      description += moreDesc;
   }
};

inline size_t countSplits(size_t sz, size_t splitsz)
{
   size_t result = sz / splitsz + ((sz % splitsz) ? 1 : 0);
   return result;
}

 /*static*/ bool PNBufferTransfer::UnitTestOnce(const char* source, size_t maxBlockSize)
{
   bool result = false;
#ifdef _DEBUG
   PNBufferTransfer test;
   size_t sz = strlen(source);

   char buffer_s[80];
   sprintf_s(buffer_s, "PNBT Unit Test of %u bytes", sz);
   TestTimer tt(buffer_s);

   // allocation size tests
   size_t cap = sz;
   test.initForCoding( cap );
   if (test.capacity() != cap) {
      tt.addResult(result, "failed CAP-TEST");
      return result;
   }

   // test the input step (raw bytes)
   const BYTE* s = reinterpret_cast<const BYTE*>(source);
   const size_t CHUNK_SIZE = maxBlockSize;
   for (size_t i =0, num = CHUNK_SIZE; num == CHUNK_SIZE; i += num) {
      num = test.addBytes( s + i, CHUNK_SIZE );
      if (test.size() != i + num) {
         tt.addResult(result, "failed ADDBYTES-TEST");
         return result;
      }
   }

   // test the buffer splitter
   int sizes[] = { 5, 1, 2, 7, 25, 29, 30, 31, 0, MAX_MESSAGE, -13 };
   for (int j = 0; j != sizeof sizes/sizeof sizes[0]; ++j) 
   {
      int SPLIT_SIZE = sizes[j];
      size_t num = test.split_buffer(SPLIT_SIZE);
      if (SPLIT_SIZE == 0) {
         if (num != 0) {
            tt.addResult(result, "failed a SIZE=0 SPLIT TEST");
            return result;
         }
      }
      else if (SPLIT_SIZE < 0) {
         if (num != countSplits(sz, MAX_MESSAGE)) // acts like a HUGE number (unsigned), so one chunk ptr and the end ptr
         {
            tt.addResult(result, "failed a SIZE<0 SPLIT TEST");
            return result;
         }
      }
      else if (num != countSplits(sz, SPLIT_SIZE))
      {
         tt.addResult(result, "failed a SPLIT TEST");
         return result;
      }
   }

   // test overall encoding process with decoding (test passes if buffers the same)
   int SPLIT_SIZE = maxBlockSize;
   int x = test.split_buffer(SPLIT_SIZE);
   PNBufferTransfer test2;
   test2.initForDecoding(x, SPLIT_SIZE);
   for (int k = 0; k < x; ++k) {
      std::string s = test.getBufferSubstring(k);
      test2.addCodedString(s);
   }
   if (!(test == test2))
   {
      tt.addResult(result, "failed ENCODE TEST");
      return result;
   }

   // test the streaming processes
   size_t total = test2.startRead();
   PNBufferTransfer test3;
   test3.initForCoding(total);
   const int BSIZE = 100; // arbitrary size
   BYTE buffer[BSIZE];
   for (size_t accumulated = 0; accumulated < total; ) {
      size_t numXfer = test2.getBytes(buffer, BSIZE);
      if (numXfer == 0)
         break;
      test3.addBytes(buffer, numXfer);
      accumulated += numXfer;
   }
   if (!(test3 == test2))
   {
      tt.addResult(result, " failed STREAM TEST");
      return result;
   }

   result = true;
   tt.addResult(result, " PASSED ALL TESTS!");
#endif //def _DEBUG
   return result;
}
