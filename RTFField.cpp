// Copyright © 2004 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "RtfField.h"

IMPLEMENT_DYNAMIC(ARtfFields, CIDSortObArray)
ARtfFields::CRtfFieldArray()
{
	m_bMustBeSorted = TRUE;
}

CRtfFieldArray::~CRtfFieldArray()
{
	DeleteAll();
}

int CRtfFieldArray::CompareObjects(CIDObject* pObject1, CIDObject* pObject2) const
{
	CRtfField* p1 = (CRtfField*)pObject1;
	CRtfField* p2 = (CRtfField*)pObject2;
	
	if (p1->m_nTextPos != p2->m_nTextPos)
		return p1->m_nTextPos - p2->m_nTextPos;
	return 0;
}

CRuntimeClass* CRtfFieldArray::GetObjectRuntimeClass() const
{
	return RUNTIME_CLASS(CRtfField);
}

CRtfField* CRtfFieldArray::GetAt(INT_PTR nIndex) const
{
	return (CRtfField*)CObArray::GetAt(nIndex);
}

INT_PTR CRtfFieldArray::Add(LPCSTR szFieldName, LPCSTR szParameters)
{
	CRtfField* pNewObject;

	pNewObject = new CRtfField();
	ASSERT_VALID(pNewObject);
	pNewObject->SetFieldName(szFieldName);
	pNewObject->SetFieldParameters(szParameters);
	return CIDSortObArray::Add(pNewObject);
}

INT_PTR CRtfFieldArray::FindField(LPCSTR szFieldName, LPCSTR szParameters)
{
	CRtfField* pObject;
	INT_PTR nIndex;

	for (nIndex = 0; nIndex < GetSize(); nIndex++) {
		pObject = GetAt(nIndex);
		ASSERT_VALID(pObject);
		if (pObject->m_stFieldName.CompareNoCase(szFieldName) == 0 && pObject->m_stFieldParameters.CompareNoCase(szParameters) == 0)
			return nIndex;
	}
	return -1;
}

INT_PTR CRtfFieldArray::FindField(LONG nStartPos, LPCSTR szFieldName)
{
	CRtfField* pObject;
	INT_PTR nIndex;

	for (nIndex = 0; nIndex < GetSize(); nIndex++) {
		pObject = GetAt(nIndex);
		ASSERT_VALID(pObject);
		if (pObject->m_stFieldName.CompareNoCase(szFieldName) == 0 && pObject->GetTextPos() >= nStartPos)
			return nIndex;
	}
	return -1;
}

INT_PTR CRtfFieldArray::ReverseFindField(LONG nStartPos, LPCSTR szFieldName)
{
	CRtfField* pObject;
	INT_PTR nIndex;

	for (nIndex = GetSize() - 1; nIndex >=0 ; nIndex--) {
		pObject = GetAt(nIndex);
		ASSERT_VALID(pObject);
		if (pObject->m_stFieldName.CompareNoCase(szFieldName) == 0 && pObject->GetTextPos() <= nStartPos)
			return nIndex;
	}
	return -1;
}

	
// CRtfFieldArray member functions

// CRtfField

IMPLEMENT_DYNAMIC(CRtfField, CIDObject)
CRtfField::CRtfField()
{
	m_nTextPos = -1;
}

CRtfField::~CRtfField()
{
}


// CRtfField member functions
CStringA CRtfField::GetFullFieldText()
{
	CStringA stRet;
	stRet.Format("{\\*\\%s %s}", m_stFieldName, m_stFieldParameters);
	return stRet;
}

BOOL CRtfField::IsFieldType(LPCSTR szFieldName)
{
	return m_stFieldName.CompareNoCase(szFieldName) == 0;
}
