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
#include "XCSoar.h"
#include "externs.h"
#include "Screen/Util.hpp"


void MapWindow::CalculateScreenPositionsGroundline(void) {
  if (FinalGlideTerrain) {
    LatLon2Screen(DerivedDrawInfo.GlideFootPrint,
		  Groundline, NUMTERRAINSWEEPS+1, 1);
  }
}


void MapWindow::DrawTerrainAbove(HDC hDC, const RECT rc, HDC buffer) {

  if (!DerivedDrawInfo.Flying) return;

  COLORREF whitecolor = RGB(0xff,0xff,0xff);
  COLORREF graycolor = RGB(0xf0,0xf0,0xf0);
  COLORREF origcolor = SetTextColor(buffer, whitecolor);

  SetBkMode(buffer, TRANSPARENT);

  SetBkColor(buffer, whitecolor);

  SelectObject(buffer, GetStockObject(WHITE_PEN));
  SetTextColor(buffer, graycolor);
  SelectObject(buffer, hAboveTerrainBrush); // hAirspaceBrushes[3] or 6
  Rectangle(buffer,rc.left,rc.top,rc.right,rc.bottom);

  SelectObject(buffer, GetStockObject(WHITE_PEN));
  SelectObject(buffer, GetStockObject(WHITE_BRUSH));
  Polygon(buffer,Groundline,NUMTERRAINSWEEPS+1);

  // need to do this to prevent drawing of colored outline
  SelectObject(buffer, GetStockObject(WHITE_PEN));
#if (WINDOWSPC<1)
    TransparentImage(hDC,
                     rc.left, rc.top,
                     rc.right-rc.left,rc.bottom-rc.top,
                     buffer,
                     rc.left, rc.top,
                     rc.right-rc.left,rc.bottom-rc.top,
                     whitecolor
                     );

#else
    TransparentBlt(hDC,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   buffer,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   whitecolor
                   );
  #endif

  // restore original color
  SetTextColor(buffer, origcolor);
  SetBkMode(buffer, OPAQUE);

}


void MapWindow::DrawGlideThroughTerrain(HDC hDC, const RECT rc) {
  HPEN hpOld;

  hpOld = (HPEN)SelectObject(hDC,
                             hpTerrainLineBg);  //sjt 02feb06 added bg line

  SelectObject(hDC,hpTerrainLineBg);
  ClipPolyline(hDC, Groundline, NUMTERRAINSWEEPS + 1, rc);
  if ((FinalGlideTerrain==1) ||
      ((!EnableTerrain || !DerivedDrawInfo.Flying) && (FinalGlideTerrain==2))) {
    SelectObject(hDC,hpTerrainLine);
    ClipPolyline(hDC, Groundline, NUMTERRAINSWEEPS + 1, rc);
  }

  if (DerivedDrawInfo.Flying && ValidTaskPoint(ActiveWayPoint)) {
    if ((DerivedDrawInfo.TerrainWarningLatitude != 0.0)
        &&(DerivedDrawInfo.TerrainWarningLongitude != 0.0)) {

      POINT sc;
      if (PointVisible(DerivedDrawInfo.TerrainWarningLongitude,
                       DerivedDrawInfo.TerrainWarningLatitude)) {
        LatLon2Screen(DerivedDrawInfo.TerrainWarningLongitude,
                      DerivedDrawInfo.TerrainWarningLatitude, sc);
        DrawBitmapIn(hDC, sc, hTerrainWarning);
      }
    }
  }

  SelectObject(hDC, hpOld);

}

