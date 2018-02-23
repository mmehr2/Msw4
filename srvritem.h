// Copyright © 2004 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


class AMswDoc;
class AMswView;

class AEmbeddedItem : public COleServerItem
{
	DECLARE_DYNAMIC(AEmbeddedItem)

public:
	AEmbeddedItem(AMswDoc* pContainerDoc, int nBeg = 0, int nEnd = -1);

	int m_nBeg;
	int m_nEnd;
	LPDATAOBJECT m_lpRichDataObj;
	AMswDoc* GetDocument() const
		{ return (AMswDoc*) COleServerItem::GetDocument(); }
	AMswView* GetView() const;

public:
	BOOL OnDrawEx(CDC* pDC, CSize& rSize, BOOL bOutput);
	virtual BOOL OnDraw(CDC* pDC, CSize& rSize);
	virtual BOOL OnGetExtent(DVASPECT dwDrawAspect, CSize& rSize);

protected:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
};


