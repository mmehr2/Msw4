// cntritem.h : interface of the CMswCntrItem class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

class AMswDoc;
class AMswView;

class CMswCntrItem : public CRichEditCntrItem
{
	DECLARE_SERIAL(CMswCntrItem)

// Constructors
public:
	CMswCntrItem(REOBJECT* preo = NULL, AMswDoc* pContainer = NULL);
		// Note: pContainer is allowed to be NULL to enable IMPLEMENT_SERIALIZE.
		//  IMPLEMENT_SERIALIZE requires the class have a constructor with
		//  zero arguments.  Normally, OLE items are constructed with a
		//  non-NULL document pointer.

// Attributes
public:
	AMswDoc* GetDocument()
		{ return (AMswDoc*)COleClientItem::GetDocument(); }
	AMswView* GetActiveView()
		{ return (AMswView*)COleClientItem::GetActiveView(); }

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMswCntrItem)
	public:
	protected:
	//}}AFX_VIRTUAL
};

/////////////////////////////////////////////////////////////////////////////
