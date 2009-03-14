/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

}
*/

#ifndef GAUGE_VARIO_H
#define GAUGE_VARIO_H
#include "StdAfx.h"

extern HWND hWndVarioWindow;

typedef struct{
  bool    InitDone;
  int     maxLabelSize;
  RECT    recBkg;
  POINT   orgText;
  double  lastValue;
  TCHAR   lastText[32];
  HBITMAP lastBitMap;
}DrawInfo_t;

class GaugeVario {
 public:
  static void Create();
  static void Destroy();
  static void Render();
  static void RenderBg();
  static void Repaint(HDC hDC);
  static void Show(bool doshow);

 private:
  static void RenderZero(void);
  static void RenderValue(int x, int y, DrawInfo_t *diValue, DrawInfo_t *diLabel, double Value, const TCHAR *Label);
  static void RenderSpeedToFly(int x, int y);
  static void RenderBallast(void);
  static void RenderBugs(void);
  static int  ValueToNeedlePos(double Value);
  static void RenderNeedle(int i, bool average, bool clear);
  static void RenderVarioLine(int i, int sink, bool clear);
  static void RenderClimb(void);

  static int xoffset;
  static int yoffset;
  static int gmax;
  static void MakePolygon(const int i);
  static void MakeAllPolygons();
  static POINT* getPolygon(const int i);
  static POINT *polys;
  static POINT *lines;
  static bool dirty;
  static HDC hdcScreen;
  static HDC hdcDrawWindow;
  static HDC hdcTemp;
  static HBITMAP hDrawBitMap;
  static RECT rc;
  static DrawInfo_t diValueTop;
  static DrawInfo_t diValueMiddle;
  static DrawInfo_t diValueBottom;
  static DrawInfo_t diLabelTop;
  static DrawInfo_t diLabelMiddle;
  static DrawInfo_t diLabelBottom;
  static HBITMAP hBitmapUnit;
  static HBITMAP hBitmapClimb;
  static POINT BitmapUnitPos;
  static POINT BitmapUnitSize;
  static HBRUSH redBrush;
  static HBRUSH blueBrush;
  static HPEN redPen;
  static HPEN bluePen;
  static HPEN redThickPen;
  static HPEN blueThickPen;
  static HPEN blankThickPen;
  static HBRUSH yellowBrush;
  static HBRUSH greenBrush;
  static HBRUSH magentaBrush;
  static HPEN yellowPen;
  static HPEN greenPen;
  static HPEN magentaPen;

};

#endif
