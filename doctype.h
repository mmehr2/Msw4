// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


class DocType
{
public:
   enum Id {
      RD_UNKNOWN  = -1,
      RD_WINWORD2 = 0,
      RD_WINWORD6 = 1,
      RD_MSW      = 2,
      RD_RICHTEXT = 3,
      RD_TEXT     = 4,
      RD_UTEXT    = 5,
      RD_OEMTEXT  = 6,
      RD_ALL      = 7,

      RD_DEFAULT  = RD_MSW,
      RD_NATIVE   = RD_RICHTEXT
   };

   int nID;
	int idStr;
	bool bRead;
	bool bWrite;
	bool bDup;
	LPCTSTR pszConverterName;
	CString GetString(int nID);
};

typedef bool (*PISFORMATFUNC)(LPCTSTR pszConverter, LPCTSTR pszPathName);
inline bool IsTextType(DocType::Id nType)
{
   return ((nType == DocType::RD_TEXT) || 
           (nType == DocType::RD_UTEXT) ||
           (nType == DocType::RD_OEMTEXT));
}

#define DOCTYPE_DOCTYPE    0
#define DOCTYPE_DESC       1
#define DOCTYPE_EXT        2
#define DOCTYPE_PROGID     3

#define DECLARE_DOCTYPE(name, b1, b2, b3, p)                {DocType::RD_##name, IDS_##name##_DOC, b1, b2, b3, p}
#define DECLARE_DOCTYPE_SYN(actname, name, b1, b2, b3, p)   {DocType::RD_##actname, IDS_##name##_DOC, b1, b2, b3, p}
#define DECLARE_DOCTYPE_NULL(name, b1, b2, b3, p)           {DocType::RD_##name, NULL, b1, b2, b3, p}

extern DocType gDocTypes[9];
extern DocType::Id GetDocTypeFromName(LPCTSTR pszPathName, DocType::Id& supposedType, CFileException& fe);
extern void ScanForConverters();
int GetIndexFromType(DocType::Id nType, bool bOpen);
DocType::Id GetTypeFromIndex(int nType, bool bOpen);
CString GetExtFromType(DocType::Id nDocType);
CString GetFileTypes(bool bOpen);
