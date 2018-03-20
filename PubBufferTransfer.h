#pragma once

#include <vector>
#include <string>

#include <afx.h>

/*
File Encoder/Decoder Object
*/

class PNBufferTransfer {
   typedef std::vector<BYTE> buffer_type;
   typedef std::vector<buffer_type::const_pointer> buffer_ptr_array;
   buffer_type buffer;
   buffer_ptr_array chunks;
public:
   PNBufferTransfer();
   ~PNBufferTransfer();

   // overall operations, return false if failure, store an error condition
   bool initForCoding(size_t cap);
   bool initForDecoding(const char* filename);

   // step 1E/5D - file load to memory
   // E-NOTE: creates memory buffer able to hold entire file + expansion capacity
   bool loadFile(const char* filename);
   // we actually need a more incremental function that can be called from the RichControl EDITSTREAM callbacks to load memory a bit at a time
   void addBytes(const BYTE* pData, size_t countBytes);

   bool saveBufferToFile(const char* filename);
   // and the EDITSTREAM callback version is ... ?

   // step 2E/4D - encode/decode BASE64 (with 4:3 buffer expansion/shrinkage)
   bool encodeBuffer();
   bool decodeBuffer();

   // step 3E - split it into chunks, respecting the chunk size (after expansion)
   // NOTE: this only generates an array of pointers into the internal buffer
   size_t split_buffer(size_t section_size); // returns # of chunks generated
   std::string getBufferSubstring(size_t n); // adds any format padding (block count, number, checksum)

   // step 3D - assemble buffer one chunk at a time
   void setTransferSize(size_t block_size, int num_blocks = 1); // ??taken from received format, no need to call
   void addCodedString( const std::string& chunk ); // strips any format padding

   friend static bool UnitTest();
   static bool UnitTest();

};
