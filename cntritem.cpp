// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $


#include "stdafx.h"
#include "cntritem.h"
#include "MswDoc.h"

IMPLEMENT_SERIAL(CMswCntrItem, CRichEditCntrItem, 0)

CMswCntrItem::CMswCntrItem(REOBJECT *preo, AMswDoc* pContainer)
	: CRichEditCntrItem(preo, pContainer)
{
}
