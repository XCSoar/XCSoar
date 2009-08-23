/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "MapWindow.h"
#include "InfoBoxLayout.h"
#include "Screen/Util.hpp"
#include "XCSoar.h" // for Appearance

extern HFONT  MapWindowFont;
extern HFONT  MapWindowBoldFont;

void MapWindow::DrawBitmapX(HDC hdc, int x, int y,
                            int sizex, int sizey,
                            HDC source,
                            int offsetx, int offsety,
                            DWORD mode) {
  if (InfoBoxLayout::scale>1) {
    StretchBlt(hdc, x, y,
               IBLSCALE(sizex),
               IBLSCALE(sizey),
               source,
               offsetx, offsety, sizex, sizey,
               mode);
  } else {
    BitBlt(hdc, x, y, sizex, sizey,
           source, offsetx, offsety, mode);
  }
}

void MapWindow::DrawBitmapIn(const HDC hdc, const POINT &sc, const HBITMAP h) {
  if (!PointVisible(sc)) return;

  SelectObject(hDCTemp, h);

  DrawBitmapX(hdc,
              sc.x-IBLSCALE(5),
              sc.y-IBLSCALE(5),
              10,10,
	      hDCTemp,0,0,SRCPAINT);
  DrawBitmapX(hdc,
              sc.x-IBLSCALE(5),
              sc.y-IBLSCALE(5),
              10,10,
              hDCTemp,10,0,SRCAND);
}



void MapWindow::_DrawLine(HDC hdc, const int PenStyle, const int width,
			  const POINT ptStart, const POINT ptEnd,
			  const COLORREF cr,
			  const RECT rc) {

  HPEN hpDash,hpOld;
  POINT pt[2];
  //Create a dot pen
  hpDash = (HPEN)CreatePen(PenStyle, width, cr);
  hpOld = (HPEN)SelectObject(hdc, hpDash);

  pt[0].x = ptStart.x;
  pt[0].y = ptStart.y;
  pt[1].x = ptEnd.x;
  pt[1].y = ptEnd.y;

  ClipPolyline(hdc, pt, 2, rc);

  SelectObject(hdc, hpOld);
  DeleteObject((HPEN)hpDash);
}


void MapWindow::DrawDashLine(HDC hdc, const int width,
			     const POINT ptStart, const POINT ptEnd, const COLORREF cr,
			     const RECT rc)
{
  int i;
  HPEN hpDash,hpOld;
  POINT pt[2];
  //Create a dot pen
  hpDash = (HPEN)CreatePen(PS_DASH, 1, cr);
  hpOld = (HPEN)SelectObject(hdc, hpDash);

  pt[0].x = ptStart.x;
  pt[0].y = ptStart.y;
  pt[1].x = ptEnd.x;
  pt[1].y = ptEnd.y;

  //increment on smallest variance
  if(abs(ptStart.x - ptEnd.x) < abs(ptStart.y - ptEnd.y)){
    pt[0].x -= width / 2;
    pt[1].x -= width / 2;
    for (i = 0; i < width; i++){
      pt[0].x += 1;
      pt[1].x += 1;
      ClipPolyline(hdc, pt, 2, rc);
    }
  } else {
    pt[0].y -= width / 2;
    pt[1].y -= width / 2;
    for (i = 0; i < width; i++){
      pt[0].y += 1;
      pt[1].y += 1;
      ClipPolyline(hdc, pt, 2, rc);
    }
  }

  SelectObject(hdc, hpOld);
  DeleteObject((HPEN)hpDash);

}


bool TextInBoxMoveInView(POINT *offset, RECT *brect){

  bool res = false;

  int LabelMargin = 4;

  offset->x = 0;
  offset->y = 0;

  if (MapWindow::MapRect.top > brect->top){
    int d = MapWindow::MapRect.top - brect->top;
    brect->top += d;
    brect->bottom += d;
    offset->y += d;
    brect->bottom -= d;
    brect->left -= d;
    offset->x -= d;
    res = true;
  }

  if (MapWindow::MapRect.right < brect->right){
    int d = MapWindow::MapRect.right - brect->right;

    if (offset->y < LabelMargin){
      int dy;

      if (d > -LabelMargin){
        dy = LabelMargin-offset->y;
        if (d > -dy)
          dy = -d;
      } else {
        int x = d + (brect->right - brect->left) + 10;

        dy = x - offset->y;

        if (dy < 0)
          dy = 0;

        if (dy > LabelMargin)
          dy = LabelMargin;
      }

      brect->top += dy;
      brect->bottom += dy;
      offset->y += dy;

    }

    brect->right += d;
    brect->left += d;
    offset->x += d;

    res = true;
  }

  if (MapWindow::MapRect.bottom < brect->bottom){
    if (offset->x == 0){
      int d = MapWindow::MapRect.bottom - brect->bottom;
      brect->top += d;
      brect->bottom += d;
      offset->y += d;
    } else
      if (offset->x < -LabelMargin){
	int d = -(brect->bottom - brect->top) - 10;
	brect->top += d;
	brect->bottom += d;
	offset->y += d;
      } else {
	int d = -(2*offset->x + (brect->bottom - brect->top));
	brect->top += d;
	brect->bottom += d;
	offset->y += d;
      }

    res = true;
  }

  if (MapWindow::MapRect.left > brect->left){
    int d = MapWindow::MapRect.left - brect->left;
    brect->right+= d;
    brect->left += d;
    offset->x += d;
    res = true;
  }

  return(res);

}

// VENTA5 now returns true if really wrote something
bool MapWindow::TextInBox(HDC hDC, const TCHAR* Value, int x, int y,
                          int size, TextInBoxMode_t Mode, bool noOverlap) {

  SIZE tsize;
  RECT brect;
  HFONT oldFont=0;
  POINT org;
  bool drawn=false;

  if ((x<MapRect.left-WPCIRCLESIZE) ||
      (x>MapRect.right+(WPCIRCLESIZE*3)) ||
      (y<MapRect.top-WPCIRCLESIZE) ||
      (y>MapRect.bottom+WPCIRCLESIZE)) {
    return drawn; // FIX Not drawn really
  }

  org.x = x;
  org.y = y;

  if (size==0) {
    size = _tcslen(Value);
  }

  HBRUSH hbOld;
  hbOld = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));

  if (Mode.AsFlag.Reachable){
    if (Appearance.IndLandable == wpLandableDefault){
      x += 5;  // make space for the green circle
    }else
      if (Appearance.IndLandable == wpLandableAltA){
	x += 0;
      }
  }

  // landable waypoint label inside white box
  if (!Mode.AsFlag.NoSetFont) {  // VENTA5 predefined font from calling function
    if (Mode.AsFlag.Border){
      oldFont = (HFONT)SelectObject(hDC, MapWindowBoldFont);
    } else {
      oldFont = (HFONT)SelectObject(hDC, MapWindowFont);
    }
  }

  GetTextExtentPoint(hDC, Value, size, &tsize);

  if (Mode.AsFlag.AlligneRight){
    x -= tsize.cx;
  } else
    if (Mode.AsFlag.AlligneCenter){
      x -= tsize.cx/2;
      y -= tsize.cy/2;
    }

  bool notoverlapping = true;

  if (Mode.AsFlag.Border || Mode.AsFlag.WhiteBorder){

    POINT offset;

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    if (Mode.AsFlag.AlligneRight)
      x -= 3;

    if (TextInBoxMoveInView(&offset, &brect)){
      x += offset.x;
      y += offset.y;
    }

    notoverlapping = checkLabelBlock(brect);

    if (!noOverlap || notoverlapping) {
      HPEN oldPen;
      if (Mode.AsFlag.Border) {
        oldPen = (HPEN)SelectObject(hDC, hpMapScale);
      } else {
        oldPen = (HPEN)SelectObject(hDC, GetStockObject(WHITE_PEN));
      }
      RoundRect(hDC, brect.left, brect.top, brect.right, brect.bottom,
                IBLSCALE(8), IBLSCALE(8));
      SelectObject(hDC, oldPen);
#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, x, y, 0, NULL, Value, size, NULL);
#else
      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
#endif
      drawn=true;
    }


  } else if (Mode.AsFlag.FillBackground) {

    POINT offset;

    brect.left = x-1;
    brect.right = brect.left+tsize.cx+1;
    brect.top = y+((tsize.cy+4)>>3);
    brect.bottom = brect.top+tsize.cy-((tsize.cy+4)>>3);

    if (Mode.AsFlag.AlligneRight)
      x -= 2;

    if (TextInBoxMoveInView(&offset, &brect)){
      x += offset.x;
      y += offset.y;
    }

    notoverlapping = checkLabelBlock(brect);

    if (!noOverlap || notoverlapping) {
      COLORREF oldColor = SetBkColor(hDC, RGB(0xff, 0xff, 0xff));
      ExtTextOut(hDC, x, y, ETO_OPAQUE, &brect, Value, size, NULL);
      SetBkColor(hDC, oldColor);
      drawn=true;
    }

  } else if (Mode.AsFlag.WhiteBold) {

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    notoverlapping = checkLabelBlock(brect);

    if (!noOverlap || notoverlapping) {
	SetTextColor(hDC,RGB(0xff,0xff,0xff));

#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, x+1, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x+2, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-1, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-2, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y+1, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y-1, 0, NULL, Value, size, NULL);
	SetTextColor(hDC,RGB(0x00,0x00,0x00));

      ExtTextOut(hDC, x, y, 0, NULL, Value, size, NULL);

#else
      ExtTextOut(hDC, x+2, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x+1, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-1, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-2, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y+1, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y-1, ETO_OPAQUE, NULL, Value, size, NULL);
	SetTextColor(hDC,RGB(0x00,0x00,0x00));

      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
#endif
      drawn=true;
    }

  } else {

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    notoverlapping = checkLabelBlock(brect);

    if (!noOverlap || notoverlapping) {
#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, x, y, 0, NULL, Value, size, NULL);
#else
      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
#endif
      drawn=true;
    }

  }

  SelectObject(hDC, hbOld);

  return drawn;

}


void MapWindow::DrawGreatCircle(HDC hdc,
                                double startLon, double startLat,
                                double targetLon, double targetLat,
				const RECT rc) {


#if OLD_GREAT_CIRCLE
  // TODO accuracy: this is actually wrong, it should recalculate the
  // bearing each step
  double distance=0;
  double distanceTotal=0;
  double Bearing;

  DistanceBearing(startLat,
                  startLon,
                  targetLat,
                  targetLon,
                  &distanceTotal,
                  &Bearing);

  distance = distanceTotal;

  if (distanceTotal==0.0) {
    return;
  }

  double d_distance = max(5000.0,distanceTotal/10);

  HPEN hpOld = (HPEN)SelectObject(hdc, hpBearing);

  POINT StartP;
  POINT EndP;
  LatLon2Screen(startLon,
                startLat,
                StartP);
  LatLon2Screen(targetLon,
                targetLat,
                EndP);

  if (d_distance>distanceTotal) {
    ClipLine(hdc, StartP, EndP, rc);
  } else {

    for (int i=0; i<= 10; i++) {

      double tlat1, tlon1;

      FindLatitudeLongitude(startLat,
                            startLon,
                            Bearing,
                            min(distance,d_distance),
                            &tlat1,
                            &tlon1);

      DistanceBearing(tlat1,
                      tlon1,
                      targetLat,
                      targetLon,
                      &distance,
                      &Bearing);

      LatLon2Screen(tlon1,
                    tlat1,
                    EndP);

      ClipLine(hdc, StartP, EndP, rc);

      StartP.x = EndP.x;
      StartP.y = EndP.y;

      startLat = tlat1;
      startLon = tlon1;

    }
  }
#else
  // Simple and this should work for PNA with display bug

  HPEN hpOld = (HPEN)SelectObject(hdc, hpBearing);
  POINT pt[2];
  LatLon2Screen(startLon,
                startLat,
                pt[0]);
  LatLon2Screen(targetLon,
                targetLat,
                pt[1]);
  ClipPolygon(hdc, pt, 2, rc, false);

#endif
  SelectObject(hdc, hpOld);
}
