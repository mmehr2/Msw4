// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "RtfHelper.h"


class ACaption {
public:
	enum State {kCCIdle, kCCNewLine, kCCReadWords, kCCReadChars, kCCOutLine, kCCOutChars};

	ACaption();
	~ACaption();
	
	bool BeginCaption(ARtfHelper& rtf);
	void ResetCaption(ARtfHelper& rtf);
	void UpdateCaption(ARtfHelper& rtf);
	void EndCaption();
	
	void SaveCCSettings();
	void LoadCCSettings();

   static std::vector<CString> GetPorts();

	int fEnabled;
   CString fPortNum;
   int fBaudRate;
   int fByteSize;
   int fParity;
   int fStopBits;

protected:
   static HANDLE OpenPort(CString& port);
   void OutputCC(int i);
	void OutputCC(char *lpOutString);
	void OutputCC(char *lpOutString, long size);
	BOOL BufferCC(char c);
	void BufferChars(char *lpData, long nCount);
	void RemoveChars(long count);

private:
   enum {MAX_PORTS=20, CHAR_CHUNK=16, LINE_WIDTH=31};

// caption output variables	
	HANDLE fPort;
	State fState;
	ARtfHelper*	fRtf;
	int fCharPos;     // current text position
	int fWordPos;     // start of current word in text
	long fCharPixel;  // global pixel location of current char
	int fLineWidth;   // number characters remaining in line
	char* fLinePtr;   // position in line
	char* fLineBuffer;// line storage
	int fCharCount;   // number of characters in line buffer
	int fCharsInWord; // number of characters in current word
   int fCuePixel;
	int fLineWords;
	
	bool fInWord, fInDelimiter;
};

///////////////////////////////////////////////////////////////////////////////
// inline functions
inline void ACaption::OutputCC(char *lpOutString) {
   this->OutputCC(lpOutString, strlen(lpOutString));
}
