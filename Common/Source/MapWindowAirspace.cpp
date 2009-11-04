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
#include "Math/FastMath.h"
#include "SettingsUser.hpp"
#include "Screen/Graphics.hpp"
#include "Compatibility/gdi.h"

void MapWindow::CalculateScreenPositionsAirspaceCircle(AIRSPACE_CIRCLE &circ) {
  circ.Visible = false;
  if (!circ.FarVisible)
    return;

  if (SettingsComputer().iAirspaceMode[circ.Type]%2 == 1) {
    double basealt;
    double topalt;

    if (circ.Base.Base != abAGL) {
      basealt = circ.Base.Altitude;
    } else {
      basealt = circ.Base.AGL + Calculated().TerrainAlt;
    }

    if (circ.Top.Base != abAGL) {
      topalt = circ.Top.Altitude;
    } else {
      topalt = circ.Top.AGL + Calculated().TerrainAlt;
    }

    if(CheckAirspaceAltitude(basealt, topalt, SettingsComputer())) {
      if (msRectOverlap(&circ.bounds, &screenbounds_latlon)
          || msRectContained(&screenbounds_latlon, &circ.bounds)) {
        if (!circ._NewWarnAckNoBrush &&
            !(SettingsMap().iAirspaceBrush[circ.Type] == NUMAIRSPACEBRUSHES-1)) {
          circ.Visible = 2;
        } else {
          circ.Visible = 1;
        }

        LonLat2Screen(circ.Location, circ.Screen);

        circ.ScreenR = DistanceMetersToScreen(circ.Radius);
      }
    }
  }
}

void MapWindow::CalculateScreenPositionsAirspaceArea(AIRSPACE_AREA &area) {
  area.Visible = false;
  if (!area.FarVisible)
    return;

  if (SettingsComputer().iAirspaceMode[area.Type]%2 == 1) {
    double basealt;
    double topalt;

    if (area.Base.Base != abAGL) {
      basealt = area.Base.Altitude;
    } else {
      basealt = area.Base.AGL + Calculated().TerrainAlt;
    }

    if (area.Top.Base != abAGL) {
      topalt = area.Top.Altitude;
    } else {
      topalt = area.Top.AGL + Calculated().TerrainAlt;
    }

    if(CheckAirspaceAltitude(basealt, topalt, SettingsComputer())) {
      if (msRectOverlap(&area.bounds, &screenbounds_latlon)
          || msRectContained(&screenbounds_latlon, &area.bounds)) {
        AIRSPACE_POINT *ap= AirspacePoint+area.FirstPoint;
        const AIRSPACE_POINT *ep= ap+area.NumPoints;
        POINT* sp= AirspaceScreenPoint+area.FirstPoint;

        while (ap < ep) {
          // JMW optimise!
          LonLat2Screen(*ap, *sp);
          ap++;
          sp++;
        }

        if (!area._NewWarnAckNoBrush &&
            !(SettingsMap().iAirspaceBrush[area.Type] == NUMAIRSPACEBRUSHES-1)) {
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

void MapWindow::ClearAirSpace(Canvas &canvas, bool fill) {
  Color whitecolor(0xff,0xff,0xff);

  canvas.set_text_color(whitecolor);
  canvas.background_transparent();
  canvas.set_background_color(whitecolor);
  canvas.white_pen();
  canvas.white_brush();
  canvas.rectangle(MapRect.left, MapRect.top, MapRect.right, MapRect.bottom);
  if (fill) {
    canvas.white_pen();
  }
}

// TODO code: optimise airspace drawing
void MapWindow::DrawAirSpace(Canvas &canvas, const RECT rc, Canvas &buffer)
{
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
        buffer.set_text_color(MapGfx.Colours[SettingsMap().iAirspaceColour[AirspaceCircle[i].Type]]);
        // get brush, can be solid or a 1bpp bitmap
        buffer.select(MapGfx.hAirspaceBrushes[SettingsMap().iAirspaceBrush[AirspaceCircle[i].Type]]);
        buffer.circle(AirspaceCircle[i].Screen.x,
                      AirspaceCircle[i].Screen.y,
                      AirspaceCircle[i].ScreenR);
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
        buffer.set_text_color(MapGfx.Colours[SettingsMap().iAirspaceColour[AirspaceArea[i].Type]]);
        buffer.select(MapGfx.hAirspaceBrushes[SettingsMap().iAirspaceBrush[AirspaceArea[i].Type]]);
        buffer.polygon(AirspaceScreenPoint+AirspaceArea[i].FirstPoint,
            AirspaceArea[i].NumPoints);
      }
    }
  }

  ////////// draw it again, just the outlines

  if (found) {
    buffer.hollow_brush();
    buffer.white_pen();
  }

  if (AirspaceCircle) {
    for(i=0;i<NumberOfAirspaceCircles;i++) {
      if (AirspaceCircle[i].Visible) {
        if (!found) {
          ClearAirSpace(buffer, false);
          found = true;
        }

        if (SettingsMap().bAirspaceBlackOutline) {
          buffer.black_pen();
        } else {
          buffer.select(MapGfx.hAirspacePens[AirspaceCircle[i].Type]);
        }

        buffer.circle(AirspaceCircle[i].Screen.x,
                      AirspaceCircle[i].Screen.y,
                      AirspaceCircle[i].ScreenR);
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

        if (SettingsMap().bAirspaceBlackOutline) {
          buffer.black_pen();
        } else {
          buffer.select(MapGfx.hAirspacePens[AirspaceArea[i].Type]);
        }

        POINT *pstart = AirspaceScreenPoint+AirspaceArea[i].FirstPoint;
        buffer.polyline(pstart, AirspaceArea[i].NumPoints);

        if (AirspaceArea[i].NumPoints>2) {
          // JMW close if open
          if ((pstart[0].x != pstart[AirspaceArea[i].NumPoints-1].x) ||
              (pstart[0].y != pstart[AirspaceArea[i].NumPoints-1].y)) {
            POINT ps[2];
            ps[0] = pstart[0];
            ps[1] = pstart[AirspaceArea[i].NumPoints-1];
            buffer.polyline(ps, 2);
          }
        }
      }
    }
  }

  if (found) {
    // need to do this to prevent drawing of colored outline
    buffer.white_pen();

    canvas.copy_transparent_white(buffer, rc);

    // restore original color
    //    SetTextColor(hDCTemp, origcolor);
    buffer.background_opaque();
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
