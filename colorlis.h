// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


class AColorMenu : public CMenu
{
public:
	AColorMenu();

public:
	static int BASED_CODE indexMap[17];
	static COLORREF GetColor(UINT id);

public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
};
