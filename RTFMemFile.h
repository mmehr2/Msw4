// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include "RtfField.h"


typedef struct symbol
{
	LPSTR szKeyword;
	LPSTR szReplace;
} Rtf_SYM;


class CRtfMemFile : public CMemFile
{
public:
	enum HeaderType
	{
		fontTable = 1,
		fileTable,
		colorTable,
		styleSheet,
		listTables,
		revTable,
		anyOtherTable,
	};
	enum RtfFieldType
	{
		kAnyFieldType  = 0,
		kBookmark      = 1,
		kPaperclip     = 2,
	};

public:
   CRtfMemFile() : fScrolling(false) {}

protected:
	void SetEndToNull();
	BOOL FindHeaderElement(int nType, size_t& nElementPos, size_t& nLength);
	CStringA GetPrintableRtfSymbol(LPCSTR szRtfSymbol);
	CStringA ParseRtfKeyword(size_t& nFilePos);

	CStringA BuildColorTable();
	COLORREF GetColorFromRtfString(LPCSTR szColor);
	size_t GetColorTablePos();

public:
	const BYTE* GetBuffer() {return m_lpBuffer;}
	BOOL IsRtf();	
	BOOL FindRtfField(int nType, size_t& nElementPos, size_t& nLength, int nDir);
	size_t GetHeaderSize();
	void BuildRtfFieldArray(ARtfFields& arrayRtfFields, size_t nHeaderSize);
	BOOL ParseColorTable();
	BOOL ResetColor(int nOldColorIndex, int nNewColorIndex);
	BOOL RebuildColorTable();
	int AddColorToColorTable(COLORREF color);
	BOOL ChangeColorInColorTable(COLORREF colorFrom, COLORREF colorTo);
	LONG GetTextPosFromFilePos(size_t nFilePos, size_t nHeaderSize);
	size_t GetFilePosFromTextPos(LONG nTextPos, size_t nHeaderSize);
	size_t FindString(LPCSTR pFind, size_t nStartPos, int nDir);
	void Insert(size_t nPos, LPCSTR lpBuf, UINT nCount);
	void Insert(size_t nPos, CStringA& stInsert);
	void Remove(size_t nPos, size_t nCount);
   void RemoveColors(COLORREF except);

   void StreamFromFile(CRichEditCtrl& edit);
	void StreamFromFile(CRichEditCtrl& edit, int format);
   void StreamToFile(CRichEditCtrl& edit, int format);
   static void SetTextColor(CRichEditCtrl& edit, COLORREF newColor, bool removeColors, bool scrolling);
   void SetTextColor(COLORREF newColor, bool removeColors);

   void DumpColors();

protected:
	CArray<COLORREF>	m_arrayColors;

public:
   bool fScrolling;
};
