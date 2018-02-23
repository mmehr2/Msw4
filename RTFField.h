// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once

#include <vector>


class ARtfField
{
public:
	ARtfField(const CStringA& name, const CStringA& params, long pos);

	long fTextPos;			// Position in the file
	CStringA	fName;
	CStringA	fParameters;

	void AdjustTextPos(long nAdjust) {fTextPos += nAdjust;}
	CStringA	GetFullFieldText() const;
	bool IsFieldType(LPCSTR szFieldName);
};

class ARtfFields : public std::vector<ARtfField>
{
public:
//	int FindField(LPCSTR szFieldName, LPCSTR szParameters);
//	int FindField(LONG nStartPos, LPCSTR szFieldName);
//	int ReverseFindField(LONG nStartPos, LPCSTR szFieldName);
};

//-----------------------------------------------------------------------------
// inline functions
inline ARtfField::ARtfField(const CStringA& name, const CStringA& params, long pos) :
   fName(name),
   fParameters(params),
   fTextPos(pos)
{
}
