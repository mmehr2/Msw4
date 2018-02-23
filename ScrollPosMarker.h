// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#pragma once


class AScriptQueue;

class AScrollPosIndicator
{
public:
   AScrollPosIndicator();
   
   void Create(const CRect& area, CDC& dc, AScriptQueue& q, int scale);
   void Update(int pos, CWnd& wnd);
   void Draw(CDC& dc, AScriptQueue& q);

private:
   void DrawBlip(int y, CDC& dc);
   int PosToPixel(int pos);
   void Reset();

   enum {kInvalidPos = -1};
   int fScale;
   int fPos;
   CRect fArea;
   int fLastY;     
   CBitmap fBlipMap;
};
