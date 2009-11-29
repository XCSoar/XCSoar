/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Screen/Graphics.hpp"
#include "AirspaceDatabase.hpp"

#include <assert.h>

void MapWindow::CalculateScreenPositionsAirspaceCircle(AIRSPACE_CIRCLE &circ) {
  assert(airspace_database != NULL);

  circ.Visible = false;
  if (!circ.FarVisible)
    return;

  if (SettingsComputer().iAirspaceMode[circ.Type]%2 == 1) {
    double basealt = ToMSL(circ.Base, Calculated().TerrainAlt);
    double topalt = ToMSL(circ.Top, Calculated().TerrainAlt);

    if (CheckAirspaceAltitude(basealt, topalt, Basic().GetAnyAltitude(),
                              SettingsComputer())) {
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
  assert(airspace_database != NULL);

  area.Visible = false;
  if (!area.FarVisible)
    return;

  if (SettingsComputer().iAirspaceMode[area.Type]%2 == 1) {
    double basealt = ToMSL(area.Base, Calculated().TerrainAlt);
    double topalt = ToMSL(area.Top, Calculated().TerrainAlt);

    if (CheckAirspaceAltitude(basealt, topalt, Basic().GetAnyAltitude(),
                              SettingsComputer())) {
      if (msRectOverlap(&area.bounds, &screenbounds_latlon)
          || msRectContained(&screenbounds_latlon, &area.bounds)) {
        const AIRSPACE_POINT *ap =
          &airspace_database->AirspacePoint[area.FirstPoint];
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
  if (airspace_database == NULL)
    return;

  for (unsigned i = 0; i < airspace_database->NumberOfAirspaceCircles; ++i) {
    AIRSPACE_CIRCLE &circle = airspace_database->AirspaceCircle[i];
    CalculateScreenPositionsAirspaceCircle(circle);
  }

  for (unsigned i = 0; i < airspace_database->NumberOfAirspaceAreas; ++i) {
    AIRSPACE_AREA &area = airspace_database->AirspaceArea[i];
    CalculateScreenPositionsAirspaceArea(area);
  }
}

void MapWindow::ClearAirSpace(Canvas &canvas, bool fill) {
  assert(airspace_database != NULL);

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

/**
 * Draws the airspace to the given canvas
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 * @param buffer The drawing buffer
 */
void
MapWindow::DrawAirSpace(Canvas &canvas, const RECT rc, Canvas &buffer)
{
  if (airspace_database == NULL)
    return;

  // TODO code: optimise airspace drawing
  bool found = false;

  // draw without border
  for (unsigned i = 0; i < airspace_database->NumberOfAirspaceCircles; ++i) {
    const AIRSPACE_CIRCLE &circle = airspace_database->AirspaceCircle[i];

    if (circle.Visible != 2)
      continue;

    if (!found) {
      ClearAirSpace(buffer, true);
      found = true;
    }

    // this color is used as the black bit
    buffer.set_text_color(MapGfx.Colours[SettingsMap().iAirspaceColour[circle.Type]]);
    // get brush, can be solid or a 1bpp bitmap
    buffer.select(MapGfx.hAirspaceBrushes[SettingsMap().iAirspaceBrush[circle.Type]]);
    buffer.circle(circle.Screen.x, circle.Screen.y, circle.ScreenR);
  }

  for (unsigned i = 0; i < airspace_database->NumberOfAirspaceAreas; ++i) {
    const AIRSPACE_AREA &area = airspace_database->AirspaceArea[i];

    if (area.Visible != 2)
      continue;

    if (!found) {
      ClearAirSpace(buffer, true);
      found = true;
    }

    // this color is used as the black bit
    buffer.set_text_color(MapGfx.Colours[SettingsMap().iAirspaceColour[area.Type]]);
    buffer.select(MapGfx.hAirspaceBrushes[SettingsMap().iAirspaceBrush[area.Type]]);
    buffer.polygon(AirspaceScreenPoint + area.FirstPoint,
                   area.NumPoints);
  }

  // draw it again, just the outlines

  if (found) {
    buffer.hollow_brush();
    buffer.white_pen();
  }

  for (unsigned i = 0; i < airspace_database->NumberOfAirspaceCircles; ++i) {
    const AIRSPACE_CIRCLE &circle = airspace_database->AirspaceCircle[i];

    if (!circle.Visible)
      continue;

    if (!found) {
      ClearAirSpace(buffer, false);
      found = true;
    }

    if (SettingsMap().bAirspaceBlackOutline)
      buffer.black_pen();
    else
      buffer.select(MapGfx.hAirspacePens[circle.Type]);

    buffer.circle(circle.Screen.x, circle.Screen.y, circle.ScreenR);
  }

  for (unsigned i = 0; i < airspace_database->NumberOfAirspaceAreas; ++i) {
    const AIRSPACE_AREA &area = airspace_database->AirspaceArea[i];

    if (!area.Visible)
      continue;

    if (!found) {
      ClearAirSpace(buffer, false);
      found = true;
    }

    if (SettingsMap().bAirspaceBlackOutline)
      buffer.black_pen();
    else
      buffer.select(MapGfx.hAirspacePens[area.Type]);

    POINT *pstart = AirspaceScreenPoint+area.FirstPoint;
    buffer.polyline(pstart, area.NumPoints);

    if (area.NumPoints > 2) {
      // JMW close if open
      if ((pstart[0].x != pstart[area.NumPoints-1].x) ||
          (pstart[0].y != pstart[area.NumPoints-1].y)) {
        POINT ps[2];
        ps[0] = pstart[0];
        ps[1] = pstart[area.NumPoints-1];
        buffer.polyline(ps, 2);
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

static bool
IsFarVisible(const AirspaceMetadata &airspace, const rectObj &rect)
{
  return msRectOverlap(&airspace.bounds, &rect) ||
    msRectContained(&rect, &airspace.bounds) ||
    msRectContained(&airspace.bounds, &rect);
}

void MapWindow::ScanVisibilityAirspace(rectObj *bounds_active) {
  if (airspace_database == NULL)
    return;

  // received when the SetTopoBounds determines the visibility
  // boundary has changed.
  // This happens rarely, so it is good pre-filtering of what is visible.
  // (saves from having to do it every screen redraw)

  for (unsigned i = 0; i < airspace_database->NumberOfAirspaceCircles; ++i) {
    AIRSPACE_CIRCLE &circle = airspace_database->AirspaceCircle[i];

    circle.FarVisible = IsFarVisible(circle, *bounds_active);
  }

  for (unsigned i = 0; i < airspace_database->NumberOfAirspaceAreas; ++i) {
    AIRSPACE_AREA &area = airspace_database->AirspaceArea[i];

    area.FarVisible = IsFarVisible(area, *bounds_active);
  }
}
