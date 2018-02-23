// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "MSW.h"
#include "caption.h"
#include "ctype.h"

#pragma warning (push)
#pragma warning (disable: 4100)  // unused variable

static char kReset[]          =  {0x3, 0xD, 0x6, 0x6, 0xD};   // ^C<cr>^F^F<cr>
static char kImmediateMode[]  =  {0x1, '2'};        // ^A2

static char gccLineBuffer[1024] = {0};

ACaption::ACaption() :
   fEnabled(false),
   fBaudRate(CBR_1200),
   fByteSize(7),
   fParity(ODDPARITY),
   fStopBits(ONESTOPBIT),
   fPort(INVALID_HANDLE_VALUE),
   fState(kCCIdle),
   fRtf(NULL),
   fCharPos(0),
   fWordPos(0),
   fCharPixel(0),
   fLineWidth(0),
   fCharCount(0),
   fCharsInWord(0),
   fCuePixel(0),
   fInWord(false),
   fInDelimiter(false),
   fLineWords(0)
{
   fLineBuffer = fLinePtr = gccLineBuffer;
}

ACaption::~ACaption() {
}

HANDLE ACaption::OpenPort(CString& port) {
   return ::CreateFile(port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, NULL, 0);
}

std::vector<CString> ACaption::GetPorts() {
   std::vector<CString> results;
   for (int i = 1; i < MAX_PORTS; ++i) {
      CString port;
      port.Format(_T("COM%d"), i);
      HANDLE h = ACaption::OpenPort(port);
      if (INVALID_HANDLE_VALUE != h) {
         results.push_back(port);
         ::CloseHandle(h);
      }
   }
   return results;
}

static void err() {
   DWORD err = ::GetLastError();
   if (err != ERROR_IO_PENDING) {
      // WriteFile failed, but isn't delayed. Report error and abort.
      TRACE("writing failed!");
   } else { // Write is pending.
      TRACE("writing pending...");
   }
}

bool ACaption::BeginCaption(ARtfHelper& rtf) {
   if (!fEnabled || (INVALID_HANDLE_VALUE != fPort))
      return true;

   fPort = this->OpenPort(fPortNum);
   if (INVALID_HANDLE_VALUE == fPort) {
      return false;
   }

   //Getting COMM settings
   DCB dcb = {0};
   ::GetCommState(fPort, &dcb);
   dcb.BaudRate = fBaudRate;
   dcb.ByteSize = fByteSize;
   dcb.Parity = fParity;
   dcb.StopBits = fStopBits;
   if (::SetCommState(fPort, &dcb)) {
      this->ResetCaption(rtf);
      fLinePtr = fLineBuffer;
      fCharCount = 0;

      this->OutputCC(kReset);
      this->OutputCC(kImmediateMode);

      this->OutputCC(fLineBuffer, fCharCount);
      return true;
   } else {
      return false;
   }
}

void ACaption::ResetCaption(ARtfHelper& rtf) {
   ASSERT(fEnabled);

   fRtf = &rtf;
   fState = kCCNewLine;
   fCharsInWord = 0;
   fInWord = true;
   fInDelimiter = false;
   fCharPos = fWordPos = fRtf->GetCurrentUserCharPos();
}

void ACaption::UpdateCaption(ARtfHelper& rtf) {
   if (!fEnabled || (fState == kCCIdle) || (INVALID_HANDLE_VALUE == fPort))
      return;

   fRtf = &rtf;
   // get global position of our current character
   fCharPixel = fRtf->GetPosFromChar(fCharPos).y;

   // get current cue position 
   fCuePixel = fRtf->GetCurrentUserPos();
    
   switch (fState) {
      case kCCIdle:
      break;

      case kCCNewLine: {
         if (fCharPixel > fCuePixel)  // nothing to do
            break;

         fCharCount = 0;
         fLinePtr = fLineBuffer;

         if (fCharPos <= fRtf->GetTextLen()) {
            fState = kCCReadWords;
            fLineWidth = LINE_WIDTH - 1;   // leave a space for padding...
            fLineWords = 0;
         } else {
            this->ResetCaption(*fRtf);
            fState = kCCOutLine;   // put blank lines if past end of document
         }
      }
      break;      

      case kCCReadWords: {
         if (fCharPos >= fRtf->GetTextLen()) {
            this->ResetCaption(*fRtf);
            fState = kCCOutLine;
            break;
         }
         
         for (int i = 0; i < CHAR_CHUNK; ++i) {
            int csize = 0;
            unsigned char c = fRtf->GetCharAt(fCharPos, csize);
            if (0 == csize) {
               this->ResetCaption(*fRtf);
            } else if (1 == csize) {
               bool delim = (c == 0x20 || c == 0x09);
               if (fInWord) {
cc_word:
                  if (!delim) {    // more of word remaining
                     if (c == 0x0D) {
                        fState = kCCOutLine;
                     } else if (0x5F == c) { // ignore script separators
                        TRACE("");
                     } else if (this->BufferCC(c)) {
                        if (fLineWords != 0) {
                           this->RemoveChars(fCharsInWord);
                           fCharPos = fWordPos;
                        }
                        fState = kCCOutLine;
                        break;
                     }
                  } else {    // transition to delimiter
                     ++fLineWords;
                     fInWord = false;
                     fCharsInWord = 0;
                     goto cc_delim;
                  }
               } else {
cc_delim:
                  if (delim) {     // more of delimiter remaining
                     if (!fInDelimiter) {
                        bool spacing = false;
                        if (spacing) {
                           if (--fLineWidth <= 0)
                              fInDelimiter = true;
                        } else if (this->BufferCC(c)) {
                           fInDelimiter = true;
                        }
                     }
                  } else {    // transition to word
                     fInWord = true;
                     fCharsInWord = 0;
                     if (fInDelimiter) {
                        fState = kCCOutLine;
                        fInDelimiter = false;
                        break;
                     }
                     fWordPos = fCharPos;   // set beginning of word
                     goto cc_word;
                  }
               }
            }
            fCharPos += csize;
            if (fState != kCCReadWords)
               break;
         }
      }
      break;

      case kCCOutLine:
      *fLinePtr++ = 0x0D;
      ++fCharCount;
      // fall through

      case kCCOutChars:
      fLinePtr = fLineBuffer;
      this->OutputCC(fLineBuffer, fCharCount);
      fState = kCCNewLine;
      break;
   }
}

void ACaption::EndCaption() {
   if (fEnabled && (INVALID_HANDLE_VALUE != fPort)) {
      ::FlushFileBuffers(fPort);

      fLinePtr = fLineBuffer;
      fCharCount = 0;
      this->BufferCC(0x0D);
      this->OutputCC(fLineBuffer, fCharCount);

      ::CloseHandle(fPort);
      fPort = INVALID_HANDLE_VALUE;
   }

   fState = kCCIdle;
}

void ACaption::SaveCCSettings() {
   VERIFY(theApp.WriteProfileString(_T("Captioning"), _T("Port"), fPortNum));
   VERIFY(theApp.WriteProfileInt(_T("Captioning"), _T("Enable"), fEnabled));
   VERIFY(theApp.WriteProfileInt(_T("Captioning"), _T("BaudRate"), fBaudRate));
   VERIFY(theApp.WriteProfileInt(_T("Captioning"), _T("ByteSize"), fByteSize));
   VERIFY(theApp.WriteProfileInt(_T("Captioning"), _T("Parity"), fParity));
   VERIFY(theApp.WriteProfileInt(_T("Captioning"), _T("StopBits"), fStopBits));
}

void ACaption::LoadCCSettings() {
   fEnabled = theApp.GetProfileInt(_T("Captioning"), _T("Enable"), fEnabled);
   fPortNum = theApp.GetProfileString(_T("Captioning"), _T("Port"), fPortNum);
   if (fPortNum.IsEmpty()) {
      std::vector<CString> ports = this->GetPorts();
      if (ports.size() > 0) {
         fPortNum = ports[0];
      }
   }
   fBaudRate = theApp.GetProfileInt(_T("Captioning"), _T("BaudRate"), fBaudRate);
   fByteSize = theApp.GetProfileInt(_T("Captioning"), _T("ByteSize"), fByteSize);
   fParity = theApp.GetProfileInt(_T("Captioning"), _T("Parity"), fParity);
   fStopBits = theApp.GetProfileInt(_T("Captioning"), _T("StopBits"), fStopBits);
}

void ACaption::OutputCC(int i) {
   ASSERT(INVALID_HANDLE_VALUE != fPort);
   DWORD written = 0;
#ifdef _DEBUG
   TRACE1("CC Output: %d\n", i);
#endif // _DEBUG
   if (!::WriteFile(fPort, (void*)&i, sizeof(i), &written, NULL)) {
      ::err();
   }
}

void ACaption::OutputCC(char* lpOutString, long size) {
   ASSERT(INVALID_HANDLE_VALUE != fPort);
   DWORD written = 0;
#ifdef _DEBUG
   TRACE("CC Output: ");
   for (int i = 0; i < size; ++i) {TRACE1("%x", lpOutString[i]);}
   TRACE("\n");
#endif // _DEBUG
   if (!::WriteFile(fPort, lpOutString, size, &written, NULL)) {
      ::err();
   }
}

BOOL ACaption::BufferCC(char c) {
   *fLinePtr++ = c;
   ++fCharCount;
   ++fCharsInWord;
   --fLineWidth;
   return (fLineWidth <= 0);
}

void ACaption::BufferChars(char *lpData, long nCount) {
   fCharCount += nCount;
   while (nCount-- > 0)
      *fLinePtr++ = *lpData++;
}

void ACaption::RemoveChars(long count) {
   fCharCount -= count;
   fLinePtr -= count;
}


#pragma warning (pop)
