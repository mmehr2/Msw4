// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "Options.h"
#include "strings.h"

const AUnit& AUnit::operator=(const AUnit& unit)
{
	m_nTPU = unit.m_nTPU;
	m_nSmallDiv = unit.m_nSmallDiv;
	m_nMediumDiv = unit.m_nMediumDiv;
	m_nLargeDiv = unit.m_nLargeDiv;
	m_nMinMove = unit.m_nMinMove;
	m_nAbbrevID = unit.m_nAbbrevID;
	m_bSpaceAbbrev = unit.m_bSpaceAbbrev;
	m_strAbbrev = unit.m_strAbbrev;
	return *this;
}

AUnit::AUnit(int nTPU, int nSmallDiv, int nMediumDiv, int nLargeDiv,
		int nMinMove, UINT nAbbrevID, BOOL bSpaceAbbrev)
{
	m_nTPU = nTPU;
	m_nSmallDiv = nSmallDiv;
	m_nMediumDiv = nMediumDiv;
	m_nLargeDiv = nLargeDiv;
	m_nMinMove = nMinMove;
	m_nAbbrevID = nAbbrevID;
	m_bSpaceAbbrev = bSpaceAbbrev;
}
