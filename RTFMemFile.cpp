// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"
#include "strings.h"
#include "RtfMemFile.h"
#include "Utils.h"
#include "ScriptQueue.h"

BOOL CRtfMemFile::IsRtf()
{
	if ((NULL == m_lpBuffer) || (m_nBufferSize < sizeof(sRtfPrefix)))
		return FALSE;
   else
      return (0 == ::memcmp(m_lpBuffer, sRtfPrefix, sizeof(sRtfPrefix)));
}

void CRtfMemFile::SetEndToNull()
{
	ASSERT_VALID(this);

   if (0 == m_nBufferSize)
   {
		ASSERT(m_lpBuffer == NULL);
		m_lpBuffer = NULL;
	}
   else
   {
	   if (m_nFileSize >= m_nBufferSize)
		   this->GrowFile(m_nFileSize + 100);

	   ASSERT(m_nBufferSize > m_nFileSize);
      ASSERT(::AfxIsValidAddress(m_lpBuffer, m_nFileSize + 1));
	   *((BYTE*)m_lpBuffer + m_nFileSize) = 0;
	   ASSERT_VALID(this);
   }
}

static bool IsTable(const char* name)
{
   char* names[] = {"header", "fonttbl", "filetbl", "colortbl", 
      "stylesheet", "listtables", "revtbl"};
   CStringA lowerName(name);
   lowerName.MakeLower();
   for (int i = 0; i < _countof(names); ++i)
      if (lowerName == names[i])
         return true;

   return false;
}

size_t CRtfMemFile::GetHeaderSize()
{
	ASSERT_VALID(this);
   ASSERT(*m_lpBuffer == '{');

   int level = 0;
	size_t nPos = 1;
   const ULONGLONG length = this->GetLength();
   char ch = *(m_lpBuffer + nPos);
   bool done = false;
   int table = 0;
   while (!done && (nPos < length) && 
      ((ch != ' ') || (level > 0)))
   {
      switch (ch)
      {
      case '{':   ++level; ++nPos; break;
      case '}':   if (--level < table) table = 0; ++nPos; break;
      case 0x0d:  ++nPos; break;
      case 0x0a:  ++nPos; break;

      case '\\':
      {  // if this is one of the header tables, we want
         // to ignore any subsequent characters
         CStringA keyword = this->ParseRtfKeyword(nPos);
         if ((level > 0) && ::IsTable(keyword))
            table = level;
         break;
      }

      default:
      if (!table && !level)
         done = true;
      else
         ++nPos;
      break;
      }

      ch = *(m_lpBuffer + nPos);
	}

   return nPos;
}

BOOL CRtfMemFile::FindHeaderElement(int nType, size_t& nElementPos, size_t& nLength)
{
	ASSERT_VALID(this);

   CStringA s;
	switch (nType)
   {
		case fontTable:      s = "{\\fonttbl"; break;
		case fileTable:      s = "{\\filetbl"; break;
		case colorTable:     s = "{\\colortbl"; break;
		case styleSheet:     s = "{\\stylesheet"; break;
		case listTables:     s = "{\\listtables"; break;
		case revTable:       s = "{\\revtbl"; break;
		case anyOtherTable:  s = gStrRtfField; break;
		default:             ASSERT(FALSE); return FALSE;
	}

   nElementPos = this->FindString(s, 0, 1);
	if (m_nFileSize == nElementPos)
		return FALSE;
	
	ASSERT(nElementPos < m_nFileSize);
	ASSERT(*(m_lpBuffer + nElementPos) == '{');
	size_t nPos = nElementPos + 1;
	int level = 1;
	while (nPos < m_nFileSize && level > 0)
   {
		if (*(m_lpBuffer + nPos) == '{')
			level++;
		if (*(m_lpBuffer + nPos) == '}')
			level--;
		nPos++;
	}
	nLength = nPos - nElementPos;
	return TRUE;
}

BOOL CRtfMemFile::FindRtfField(int nType, size_t& nElementPos, size_t& nLength, int nDir)
{
	ASSERT_VALID(this);
   CStringA s;
   switch (nType)
   {
		case kAnyFieldType:   s = gStrRtfField; break;
		case kBookmark:       s = CStringA(gStrRtfField) + gStrBookmark; break;
		case kPaperclip:      s = CStringA(gStrRtfField) + gStrPaperclip; break;
		default:              ASSERT(FALSE); return FALSE;
	}

   nElementPos = this->FindString(s, nElementPos, nDir);
	nLength = 0;
	if (nElementPos >= m_nFileSize || nElementPos == 0)
		return FALSE;

   ASSERT(*(m_lpBuffer + nElementPos) == '{');
	size_t nPos = nElementPos + 1;
	int level = 1;
	while (nPos < m_nFileSize && level > 0)
   {
		if (*(m_lpBuffer + nPos) == '{')
			level++;
		if (*(m_lpBuffer + nPos) == '}')
			level--;
		nPos++;
	}
	nLength = nPos - nElementPos;
	return TRUE;
}

void CRtfMemFile::BuildRtfFieldArray(ARtfFields& fields, size_t nHeaderSize)
{
	const CStringA tag = gStrRtfField;
	fields.clear();

   for (size_t nLen = 0, nPos = 0; this->FindRtfField(kAnyFieldType, nPos, nLen, 1); ++nPos)
   {
      CStringA field((char*)(m_lpBuffer + nPos + tag.GetLength()), nLen - tag.GetLength() - 1);
	   char* temp_tok = NULL;
      char* pName = ::strtok_s((char*)(const char*)field, " }", &temp_tok);
   	char* pParams = ::strtok_s(NULL, "}", &temp_tok);
      
      fields.push_back(ARtfField(pName, pParams, 
         this->GetTextPosFromFilePos(nPos, nHeaderSize)));
	}
}

CStringA CRtfMemFile::GetPrintableRtfSymbol(LPCSTR szRtfSymbol)
{
   static const Rtf_SYM rgsymRtf[] = {
      {"par",        "\n"},
	   {"tab",        "\t"},
	   {"ldblquote",  "\"\""},
	   {"rdblquote",  "\"\""},
	   {"{",          "{"},
	   {"}",          "}"},
      {"\\",         "\\"}
   };

   CStringA stRet;
	for (int nSym = 0; nSym < mCountOf(rgsymRtf); nSym++)
   {
      if (::_stricmp(szRtfSymbol, rgsymRtf[nSym].szKeyword) == 0)
      {
			stRet = rgsymRtf[nSym].szReplace;
			break;
		}
	}
	return stRet;
}

CStringA CRtfMemFile::ParseRtfKeyword(size_t& nFilePos)
{
	ASSERT(*(m_lpBuffer + nFilePos) == '\\');

   CStringA stRet;
	if (*(m_lpBuffer + nFilePos) == '\\')
		nFilePos++;

   if (!isalpha(*(m_lpBuffer + nFilePos)))
   {
		stRet.AppendChar(*(m_lpBuffer + nFilePos));
		nFilePos++;
		return stRet;
	}

   while (isalpha(*(m_lpBuffer + nFilePos)) && nFilePos < m_nFileSize)
   {
      stRet.AppendChar(*(m_lpBuffer + nFilePos));
      nFilePos++;
   }

   if (*(m_lpBuffer + nFilePos) == '-' && nFilePos < m_nFileSize)
   {
      stRet.AppendChar(*(m_lpBuffer + nFilePos));
      nFilePos++;
   }

   while (isdigit(*(m_lpBuffer + nFilePos)) && nFilePos < m_nFileSize)
   {
		stRet.AppendChar(*(m_lpBuffer + nFilePos));
		nFilePos++;
	}

   if (*(m_lpBuffer + nFilePos) == ' ' && nFilePos < m_nFileSize)
		nFilePos++;

	return stRet;
}

LONG CRtfMemFile::GetTextPosFromFilePos(size_t nFilePos, size_t nHeaderSize)
{
	LONG nTextPos = 0;
	if (nFilePos <= nHeaderSize)
		return 0;

	size_t nPos = nHeaderSize;
   int level = 0;
	while (nPos < nFilePos)
   {  // skip {} sections
      if (*(m_lpBuffer + nPos) == '{')
      {
		   while (*(m_lpBuffer + nPos) == '{')
         {
			   level = 1;
			   nPos++;
			   while (nPos < nFilePos && level > 0)
            {
				   if (*(m_lpBuffer + nPos) == '{')
					   level++;
				   if (*(m_lpBuffer + nPos) == '}')
					   level--;
				   nPos++;
			   }

            if (level > 0)
				   return nTextPos;
		   }
      }
		// skip Rtf Keywords begining with backslash (\) characters
		else if (*(m_lpBuffer + nPos) == '\\')
      {
	      CStringA stRtfKeyWord = this->ParseRtfKeyword(nPos);
			nTextPos += this->GetPrintableRtfSymbol(stRtfKeyWord).GetLength();
		}
		// Skip cr and lf - they are noise characters...
		else if (*(m_lpBuffer + nPos) != 0x0d && *(m_lpBuffer + nPos) != 0x0a)
      {
         nTextPos++;
         nPos++;
      }
      else
		   nPos++;
	}
	return nTextPos;
}

size_t CRtfMemFile::GetFilePosFromTextPos(LONG nTextPos, size_t nHeaderSize)
{
	size_t nFilePos = nHeaderSize;
	LONG nPos = 0;
	int level = 0;
	while (nPos < nTextPos)
   {  // skip {} sections
		if (*(m_lpBuffer + nFilePos) == '{')
      {
			level = 1;
			nFilePos++;		// skip the bracket
			while (nFilePos < m_nFileSize && level > 0)
         {
				if (*(m_lpBuffer + nFilePos) == '{')
					level++;
				if (*(m_lpBuffer + nFilePos) == '}')
					level--;
				nFilePos++;
			}
		}
      // skip Rtf Keywords begining with backslash (\) characters
		else if (*(m_lpBuffer + nFilePos) == '\\')
      {
      	CStringA stRtfKeyWord = this->ParseRtfKeyword(nFilePos);
			nPos += this->GetPrintableRtfSymbol(stRtfKeyWord).GetLength();
		}
      // Skip cr and lf - they are noise characters...
		else if (*(m_lpBuffer + nFilePos) != 0x0d && *(m_lpBuffer + nFilePos) != 0x0a)
      {
         nFilePos++;
         nPos++;
      }
      else
         nFilePos++;
	}
	return nFilePos;
}

size_t CRtfMemFile::FindString(LPCSTR pFind, size_t nStartPos, int nDir)
{
   size_t nBytes = ::strlen(pFind);
	size_t nMaxLen = m_nFileSize - nBytes;

	ASSERT_VALID(this);
   if (nBytes > m_nFileSize)
		return m_nFileSize;
	else if (nStartPos >= m_nFileSize && nDir >= 0)
		return m_nFileSize;
   else if (nStartPos == 0 && nDir < 0)
		return m_nFileSize;

   this->SetEndToNull();

	if (nDir >= 0)
   {
		while (nStartPos < nMaxLen)
      {
			// Just check first one to begin with
			if (*(m_lpBuffer + nStartPos) == *pFind)
         {
            if (::memcmp(m_lpBuffer + nStartPos, pFind, nBytes) == 0)
					return nStartPos;
			}
			nStartPos++;
		}
		return m_nFileSize;
	}
   else
   {
		while (nStartPos > 0)
      {
			// Just check first one to begin with
			if (*(m_lpBuffer + nStartPos) == *pFind)
         {
				if (::memcmp(m_lpBuffer + nStartPos, pFind, nBytes) == 0)
					return nStartPos;
			}
			nStartPos--;
		}
		return m_nFileSize;
	}
}

// CRtfMemFile member functions
void CRtfMemFile::Insert(size_t nPos, LPCSTR lpBuf, UINT nCount)
{
	ASSERT_VALID(this);

	if (nCount == 0)
		return;

	ASSERT(lpBuf != NULL);
   ASSERT(::AfxIsValidAddress(lpBuf, nCount, FALSE));

	if (lpBuf == NULL)
      ::AfxThrowInvalidArgException();

	// Can not insert beyond the end of the current file
	if (nPos > (int)m_nFileSize)
		::AfxThrowFileException(CFileException::endOfFile);

	if (m_nFileSize + nCount > m_nBufferSize)
		this->GrowFile(m_nFileSize + nCount);

	ASSERT(m_nFileSize + nCount <= m_nBufferSize);

   ::memmove(m_lpBuffer + nPos + nCount, m_lpBuffer + nPos, m_nFileSize - nPos);
	Memcpy(m_lpBuffer + nPos, (BYTE*)lpBuf, nCount);

	m_nFileSize += nCount;
	this->SetEndToNull();

	ASSERT_VALID(this);
}

void CRtfMemFile::Insert(size_t nPos, CStringA& stInsert)
{
	this->Insert(nPos, stInsert, stInsert.GetLength());
}

void CRtfMemFile::Remove(size_t nPos, size_t nCount)
{
	ASSERT_VALID(this);

	if (nCount == 0)
		return;

	// Can not remove beyond the end of the current file
	if (nPos > (int)m_nFileSize)
		::AfxThrowFileException(CFileException::endOfFile);

	if (nPos + nCount > m_nFileSize)
		m_nFileSize = nPos;
	else
   {
		::memmove(m_lpBuffer + nPos, m_lpBuffer + nPos + nCount, m_nFileSize - (nPos + nCount));
		m_nFileSize -= nCount;
	}

	this->SetEndToNull();
	ASSERT_VALID(this);
}

CStringA CRtfMemFile::BuildColorTable()
{
	// The first color is skipped since it is the default color
	CStringA stColorTable = "{\\colortbl;";
	for (int i = 1; i < m_arrayColors.GetCount(); i++)
   {
	   COLORREF color = m_arrayColors.GetAt(i);

      CStringA stColor;
		stColor.Format("\\red%d\\green%d\\blue%d;", GetRValue(color), GetGValue(color), GetBValue(color));
		stColorTable += stColor;
	}

	stColorTable += "}";
	return stColorTable;
}

COLORREF CRtfMemFile::GetColorFromRtfString(LPCSTR szColor)
{
	CStringA stColor = szColor;
	stColor.MakeLower();

   int nRed = 0, nBlue = 0, nGreen = 0;
	size_t nPos = stColor.Find("red");
	if (nPos >= 0)
      nRed = ::strtol(szColor + nPos + 3, NULL, 10);
	nPos = stColor.Find("green");
	if (nPos >= 0)
		nGreen = ::strtol(szColor + nPos + 5, NULL, 10);
	nPos = stColor.Find("blue");
	if (nPos >= 0)
		nBlue = ::strtol(szColor + nPos + 4, NULL, 10);

   return RGB(nRed, nGreen, nBlue);
}

BOOL CRtfMemFile::ParseColorTable()
{
	ASSERT_VALID(this);
   m_arrayColors.RemoveAll();

   m_arrayColors.Add(0);	// Add default color

   size_t nColorPos = 0;
	size_t nColorSize = 0;
	if (!this->FindHeaderElement(colorTable, nColorPos, nColorSize))
		return TRUE;

	CStringA stColorTable;
   stColorTable.SetString(LPCSTR(m_lpBuffer + nColorPos), (int)nColorSize);
	// Find the first semi-colon
	nColorPos = stColorTable.Find(";");
	ASSERT_VALID(this);
   int nStringPos = 0;
	while ((nStringPos = stColorTable.Find(";", (int)nColorPos + 1)) > 0)
   {
   	CStringA stColor = stColorTable.Mid((int)nColorPos, (int)(nStringPos - nColorPos));
		m_arrayColors.Add(this->GetColorFromRtfString(stColor));
		nColorPos = nStringPos + 1;
	}
   return TRUE;
}

size_t CRtfMemFile::GetColorTablePos()
{
	ASSERT_VALID(this);

   size_t nElementPos = 0, nElementLength = 0;
	if (this->FindHeaderElement(styleSheet, nElementPos, nElementLength))
		return nElementPos;
	else if (this->FindHeaderElement(listTables, nElementPos, nElementLength))
		return nElementPos;
	else if (this->FindHeaderElement(revTable, nElementPos, nElementLength))
		return nElementPos;
	else if (this->FindHeaderElement(fileTable, nElementPos, nElementLength))
		return nElementPos + nElementLength;
	else if (this->FindHeaderElement(fontTable, nElementPos, nElementLength))
		return nElementPos + nElementLength;
	else if (this->FindHeaderElement(anyOtherTable, nElementPos, nElementLength))
		return nElementPos;
	else 
      return this->GetHeaderSize();
}

BOOL CRtfMemFile::ResetColor(INT_PTR nOldColorIndex, INT_PTR nNewColorIndex)
{
	ASSERT_VALID(this);
	this->SetEndToNull();

   size_t nPos = 0;
   CStringA stNewColorIndex, stOldColorIndex;
	stNewColorIndex.Format("\\cf%d", nNewColorIndex);
	stOldColorIndex.Format("\\cf%d", nOldColorIndex);
	if (nOldColorIndex == 0)
   {  // check header
      nPos = this->FindString("\\cf", 0, 1);
      const size_t headerSize = this->GetHeaderSize();
		if ((m_nFileSize == nPos) || (nPos >= headerSize))
      {  // can't find the default color in the header
         if (' ' == m_lpBuffer[headerSize])
         {  // this is a valid RTF document; the character
            // after the header is a space
            nPos = headerSize - 1;
         }
         else
         {  // invalid RTF document. Inserting at the end 
            // of the header will cause the file to be 
            // unreadable
            nPos = this->FindString("\\viewkind", 0, 1);
            ASSERT((m_nFileSize != nPos) && (nPos < headerSize));
         }
         Insert(nPos, stNewColorIndex);
      }
	}

	if (nNewColorIndex == 0)
   {  // remove old color index from header
		nPos = this->FindString(stOldColorIndex, 0, 1);
		if (nPos >= 0 && nPos < this->GetHeaderSize())
			Remove(nPos, stOldColorIndex.GetLength());
	}

   nPos = 0;
	while (m_nFileSize != (nPos = this->FindString(stOldColorIndex, nPos, 1)))
   {
		Remove(nPos, stOldColorIndex.GetLength());
		Insert(nPos, stNewColorIndex);
	}

   return TRUE;
}

BOOL CRtfMemFile::RebuildColorTable()
{
	this->SetEndToNull();

	CStringA stColorTable = this->BuildColorTable();
   size_t nColorPos = 0, nColorLength = 0;
	if (this->FindHeaderElement(colorTable, nColorPos, nColorLength))
		this->Remove(nColorPos, (UINT)nColorLength);
	else
		nColorPos = this->GetColorTablePos(); // FIND POS OF NEW COLOR TABLE;

   this->Insert(nColorPos, stColorTable, stColorTable.GetLength());

   return TRUE;
}

INT_PTR CRtfMemFile::AddColorToColorTable(COLORREF color)
{
	INT_PTR nRet = m_arrayColors.Add(color);
	this->RebuildColorTable();
	return nRet;
}

BOOL CRtfMemFile::ChangeColorInColorTable(COLORREF colorFrom, COLORREF colorTo)
{
	BOOL bRebuild = FALSE;

	this->SetEndToNull();
	this->ParseColorTable();
	for (int i = 0; i < m_arrayColors.GetSize(); i++)
   {
		if (m_arrayColors.GetAt(i) == colorFrom)
      {
			m_arrayColors.SetAt(i, colorTo);
			bRebuild = TRUE;
		}
	}

   if (bRebuild)
		this->RebuildColorTable();

   return TRUE;
}

void CRtfMemFile::RemoveColors(COLORREF except)
{ // remove all colors except 'except'
   this->DumpColors();

   bool changed = false;
   for (int i = 0; i < m_arrayColors.GetCount(); ++i)
   {
	   if (m_arrayColors.GetAt(i) != except)
      {
		   m_arrayColors.SetAt(i, except);
         changed = true;
      }
   }

   if (changed)
      this->RebuildColorTable();

   this->DumpColors();
}

static DWORD CALLBACK MemFileOutCallBack(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   ((CFile*)dwCookie)->Write(pbBuff, cb);
   *pcb = cb;
   return 0;
}

void CRtfMemFile::StreamToFile(CRichEditCtrl& edit, int format)
{
   EDITSTREAM editStream;
   editStream.dwCookie = (DWORD_PTR)this;
   editStream.pfnCallback = MemFileOutCallBack; 
   edit.StreamOut(format, editStream);
}

static DWORD CALLBACK MemFileInCallBack(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   ASSERT(NULL != dwCookie);
   ASSERT(NULL != pbBuff);
   ASSERT(NULL != pcb);

   CRtfMemFile* pFile = (CRtfMemFile*)dwCookie;
	ASSERT_VALID(pFile);
   if (pFile->fScrolling) {
      // use less of the buffer, just in case we need to replace some page breaks with paragraph tags below
      cb -= (cb / 4);
   }
	*pcb = pFile->Read(pbBuff, cb);

   if (pFile->fScrolling)
   {  // remove page breaks. FormatRange stops at page breaks,
      // causing big problems in scroll mode. An additional complication - 
      // page breaks display as a blank line in edit mode, but not at all
      // in scroll mode, so we need to make adjustments...
      
      //!!! UNICODE RTF text is not handled here. Later.
      pbBuff[*pcb] = 0;
      char pageBreakTag[] = "\\page";
      char paragraphTag[] = "\\par\r\n";
      const int breakSize = ::strlen(pageBreakTag);
      for (char *pageBreak = NULL, *start = (char*)pbBuff; 
         (NULL != (pageBreak = ::strstr(start, pageBreakTag))) &&
         (pageBreak < (char*)(pbBuff + *pcb)); 
         start = pageBreak)
      {
         if (' ' != pageBreak[breakSize]) {
            ::memmove((void*)(pageBreak + 1), pageBreak, ((char*)pbBuff + *pcb) - pageBreak);
            *pcb += 1;
         }
         ::memcpy((void*)pageBreak, (void*)paragraphTag, breakSize + 1);
      }
   }

   return 0;
}

void CRtfMemFile::StreamFromFile(CRichEditCtrl& edit, int format)
{
   EDITSTREAM es;
	es.dwCookie = (DWORD_PTR)this;
   es.pfnCallback = MemFileInCallBack; 
	this->SeekToBegin();

	edit.StreamIn(format, es);
}

void CRtfMemFile::StreamFromFile(CRichEditCtrl& edit)
{  // Stream File contents from mem file
   // First, see if this is a text file or RTF file
	this->SeekToBegin();
   TCHAR buffer[16] = {0};
   this->Read(buffer, sizeof(buffer));
   const int format = (0 == memcmp(buffer, sRtfPrefix, 
      sizeof(sRtfPrefix))) ? SF_RTF : SF_TEXT;

   this->StreamFromFile(edit, format);
}

void CRtfMemFile::SetTextColor(CRichEditCtrl& edit, COLORREF newColor, bool removeColors, bool scrolling)
{
   CWaitCursor wait; // this could take a while

   CRtfMemFile file;
   file.fScrolling = scrolling;
   file.StreamToFile(edit, SF_RTF);
   ASSERT(file.GetLength() <= LONG_MAX);  // support only up to 2 GB
   file.SetTextColor(newColor, removeColors);
   file.StreamFromFile(edit, SF_RTF);

   edit.SetBackgroundColor(FALSE, (kWhite == newColor) ? kBlack : kWhite);
}

void CRtfMemFile::SetTextColor(COLORREF newColor, bool removeColors)
{
   this->ParseColorTable();
   INT_PTR i = this->AddColorToColorTable(newColor);
   this->ResetColor(0, i);

   this->ChangeColorInColorTable((kWhite == newColor) ? kBlack : kWhite, newColor);

   if (removeColors)
      this->RemoveColors(newColor);
}

void CRtfMemFile::DumpColors()
{
#ifdef _DEBUG
   CStringA stColorTable;
	size_t nColorPos = 0, nColorSize = 0;
	if (this->FindHeaderElement(colorTable, nColorPos, nColorSize))
		stColorTable.SetString(LPCSTR(m_lpBuffer + nColorPos), (int)nColorSize);
	TRACE("%s ", stColorTable);
	for (int i = 0; i < m_arrayColors.GetCount(); i++)
   {
	   COLORREF cr = m_arrayColors.GetAt(i);
		TRACE("RGB(%d, %d, %d), ", GetRValue(cr), GetGValue(cr), GetBValue(cr));
	}
	TRACE("\n");
#endif // _DEBUG
}
