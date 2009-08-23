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
#include "Airspace.h"
#include "XCSoar.h"
#include "externs.h"
#include "Screen/Util.hpp"
#include "Math/FastMath.h"


void MapWindow::CalculateScreenPositionsAirspaceCircle(AIRSPACE_CIRCLE &circ) {
  circ.Visible = false;
  if (!circ.FarVisible) return;
  if (iAirspaceMode[circ.Type]%2 == 1) {
    double basealt;
    double topalt;
    if (circ.Base.Base != abAGL) {
      basealt = circ.Base.Altitude;
    } else {
      basealt = circ.Base.AGL + DerivedDrawInfo.TerrainAlt;
    }
    if (circ.Top.Base != abAGL) {
      topalt = circ.Top.Altitude;
    } else {
      topalt = circ.Top.AGL + DerivedDrawInfo.TerrainAlt;
    }
    if(CheckAirspaceAltitude(basealt, topalt)) {
      if (msRectOverlap(&circ.bounds, &screenbounds_latlon)
          || msRectContained(&screenbounds_latlon, &circ.bounds)) {

	if (!circ._NewWarnAckNoBrush &&
	    !(iAirspaceBrush[circ.Type] == NUMAIRSPACEBRUSHES-1)) {
	  circ.Visible = 2;
	} else {
	  circ.Visible = 1;
	}

        LatLon2Screen(circ.Longitude,
                      circ.Latitude,
                      circ.Screen);
        circ.ScreenR = iround(circ.Radius*ResMapScaleOverDistanceModify);
      }
    }
  }
}

void MapWindow::CalculateScreenPositionsAirspaceArea(AIRSPACE_AREA &area) {
  area.Visible = false;
  if (!area.FarVisible) return;
  if (iAirspaceMode[area.Type]%2 == 1) {
    double basealt;
    double topalt;
    if (area.Base.Base != abAGL) {
      basealt = area.Base.Altitude;
    } else {
      basealt = area.Base.AGL + DerivedDrawInfo.TerrainAlt;
    }
    if (area.Top.Base != abAGL) {
      topalt = area.Top.Altitude;
    } else {
      topalt = area.Top.AGL + DerivedDrawInfo.TerrainAlt;
    }
    if(CheckAirspaceAltitude(basealt, topalt)) {
      if (msRectOverlap(&area.bounds, &screenbounds_latlon)
          || msRectContained(&screenbounds_latlon, &area.bounds)) {
        AIRSPACE_POINT *ap= AirspacePoint+area.FirstPoint;
        const AIRSPACE_POINT *ep= ap+area.NumPoints;
        POINT* sp= AirspaceScreenPoint+area.FirstPoint;
        while (ap < ep) {
	  // JMW optimise!
            LatLon2Screen(ap->Longitude,
                          ap->Latitude,
                          *sp);
            ap++;
            sp++;
        }

	if (!area._NewWarnAckNoBrush &&
	    !(iAirspaceBrush[area.Type] == NUMAIRSPACEBRUSHES-1)) {
	  area.Visible = 2;
	} else {
	  area.Visible = 1;
	}
      }
    }
  }
}

void MapWindow::CalculateScreenPositionsAirspace() {


  if (AirspaceCircle) {
    for (AIRSPACE_CIRCLE* circ = AirspaceCircle;
         circ < AirspaceCircle+NumberOfAirspaceCircles; circ++) {
      CalculateScreenPositionsAirspaceCircle(*circ);
    }
  }
  if (AirspaceArea) {
    for(AIRSPACE_AREA *area = AirspaceArea;
        area < AirspaceArea+NumberOfAirspaceAreas; area++) {
      CalculateScreenPositionsAirspaceArea(*area);
    }
  }
}


void MapWindow::ClearAirSpace(HDC dc, bool fill) {
  COLORREF whitecolor = RGB(0xff,0xff,0xff);

  SetTextColor(dc, whitecolor);
  SetBkMode(dc, TRANSPARENT);
  SetBkColor(dc, whitecolor);
  SelectObject(dc, GetStockObject(WHITE_PEN));
  SelectObject(dc, GetStockObject(WHITE_BRUSH));
  Rectangle(dc, MapRect.left, MapRect.top, MapRect.right, MapRect.bottom);
  if (fill) {
    SelectObject(dc, GetStockObject(WHITE_PEN));
  }
}

// TODO code: optimise airspace drawing
void MapWindow::DrawAirSpace(HDC hdc, const RECT rc, HDC buffer)
{
  COLORREF whitecolor = RGB(0xff,0xff,0xff);
  unsigned int i;

  bool found = false;

  if (AirspaceCircle) {
    // draw without border
    for(i=0;i<NumberOfAirspaceCircles;i++) {
      if (AirspaceCircle[i].Visible==2) {
	if (!found) {
          ClearAirSpace(buffer, true);
	  found = true;
	}
        // this color is used as the black bit
        SetTextColor(buffer,
                     Colours[iAirspaceColour[AirspaceCircle[i].Type]]);
        // get brush, can be solid or a 1bpp bitmap
        SelectObject(buffer,
                     hAirspaceBrushes[iAirspaceBrush[AirspaceCircle[i].Type]]);
        Circle(buffer,
               AirspaceCircle[i].Screen.x ,
               AirspaceCircle[i].Screen.y ,
               AirspaceCircle[i].ScreenR ,rc, true, true);
      }
    }
  }

  if (AirspaceArea) {
    for(i=0;i<NumberOfAirspaceAreas;i++) {
      if(AirspaceArea[i].Visible ==2) {
	if (!found) {
	  ClearAirSpace(buffer, true);
	  found = true;
	}
        // this color is used as the black bit
        SetTextColor(buffer,
                     Colours[iAirspaceColour[AirspaceArea[i].Type]]);
        SelectObject(buffer,
                     hAirspaceBrushes[iAirspaceBrush[AirspaceArea[i].Type]]);
        ClipPolygon(buffer,
                    AirspaceScreenPoint+AirspaceArea[i].FirstPoint,
                    AirspaceArea[i].NumPoints, rc, true);
      }
    }
  }

  ////////// draw it again, just the outlines

  if (found) {
    SelectObject(buffer, GetStockObject(HOLLOW_BRUSH));
    SelectObject(buffer, GetStockObject(WHITE_PEN));
  }

  if (AirspaceCircle) {
    for(i=0;i<NumberOfAirspaceCircles;i++) {
      if (AirspaceCircle[i].Visible) {
	if (!found) {
	  ClearAirSpace(buffer, false);
	  found = true;
	}
        if (bAirspaceBlackOutline) {
          SelectObject(buffer, GetStockObject(BLACK_PEN));
        } else {
          SelectObject(buffer, hAirspacePens[AirspaceCircle[i].Type]);
        }
        Circle(buffer,
               AirspaceCircle[i].Screen.x ,
               AirspaceCircle[i].Screen.y ,
               AirspaceCircle[i].ScreenR ,rc, true, false);
      }
    }
  }

  if (AirspaceArea) {
    for(i=0;i<NumberOfAirspaceAreas;i++) {
      if(AirspaceArea[i].Visible) {
	if (!found) {
	  ClearAirSpace(buffer, false);
	  found = true;
	}
        if (bAirspaceBlackOutline) {
          SelectObject(buffer, GetStockObject(BLACK_PEN));
        } else {
          SelectObject(buffer, hAirspacePens[AirspaceArea[i].Type]);
        }

	POINT *pstart = AirspaceScreenPoint+AirspaceArea[i].FirstPoint;
        ClipPolygon(buffer, pstart,
                    AirspaceArea[i].NumPoints, rc, false);

	if (AirspaceArea[i].NumPoints>2) {
	  // JMW close if open
	  if ((pstart[0].x != pstart[AirspaceArea[i].NumPoints-1].x) ||
	      (pstart[0].y != pstart[AirspaceArea[i].NumPoints-1].y)) {
	    POINT ps[2];
	    ps[0] = pstart[0];
	    ps[1] = pstart[AirspaceArea[i].NumPoints-1];
            ClipPolyline(buffer, ps, 2, rc);
	  }
	}

      }
    }
  }

  if (found) {
    // need to do this to prevent drawing of colored outline
    SelectObject(buffer, GetStockObject(WHITE_PEN));
#if (WINDOWSPC<1)
    TransparentImage(hdc,
                     rc.left, rc.top,
                     rc.right-rc.left,rc.bottom-rc.top,
                     buffer,
                     rc.left, rc.top,
                     rc.right-rc.left,rc.bottom-rc.top,
                     whitecolor
                     );

#else
    TransparentBlt(hdc,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   buffer,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   whitecolor
                   );
  #endif
    // restore original color
    //    SetTextColor(hDCTemp, origcolor);
    SetBkMode(buffer, OPAQUE);
  }
}


void MapWindow::ScanVisibilityAirspace(rectObj *bounds_active) {
  // received when the SetTopoBounds determines the visibility
  // boundary has changed.
  // This happens rarely, so it is good pre-filtering of what is visible.
  // (saves from having to do it every screen redraw)
  const rectObj bounds = *bounds_active;

  if (AirspaceCircle) {
    for (AIRSPACE_CIRCLE* circ = AirspaceCircle;
         circ < AirspaceCircle+NumberOfAirspaceCircles; circ++) {
      circ->FarVisible =
        (msRectOverlap(&circ->bounds, bounds_active) == MS_TRUE) ||
        (msRectContained(bounds_active, &circ->bounds) == MS_TRUE) ||
        (msRectContained(&circ->bounds, bounds_active) == MS_TRUE);
    }
  }

  if (AirspaceArea) {
    for(AIRSPACE_AREA *area = AirspaceArea;
        area < AirspaceArea+NumberOfAirspaceAreas; area++) {
      area->FarVisible =
        (msRectOverlap(&area->bounds, bounds_active) == MS_TRUE) ||
        (msRectContained(bounds_active, &area->bounds) == MS_TRUE) ||
        (msRectContained(&area->bounds, bounds_active) == MS_TRUE);
    }
  }
}
