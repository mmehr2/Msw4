// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


class ABuyNowButton : public CButton
{
   DECLARE_DYNAMIC(ABuyNowButton)

public:
   ABuyNowButton() {}
   virtual ~ABuyNowButton() {}

protected:
	CFont fFont;

private:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	DECLARE_MESSAGE_MAP()
};


