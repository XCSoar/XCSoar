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

#include "MapWindow.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"

#include <stdio.h>

void MapWindow::CalculateScreenPositionsGroundline(void) {
  if (SettingsComputer().FinalGlideTerrain)
    LonLat2Screen(Calculated().GlideFootPrint,
                  Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1, 1);
}

/**
 * Draw the final glide groundline (and shading) to the buffer
 * and copy the transparent buffer to the canvas
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 * @param buffer The drawing buffer
 */
void MapWindow::DrawTerrainAbove(Canvas &canvas, const RECT rc, Canvas &buffer) 
{
  if (!Basic().flight.Flying)
    return;

  buffer.background_transparent();
  buffer.set_background_color(Color::WHITE);

  buffer.white_pen();
  buffer.set_text_color(Color(0xf0,0xf0,0xf0));
  buffer.select(MapGfx.hAboveTerrainBrush); // hAirspaceBrushes[3] or 6
  buffer.rectangle(rc.left, rc.top, rc.right, rc.bottom);

  buffer.white_pen();
  buffer.white_brush();
  buffer.polygon(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1);

  // need to do this to prevent drawing of colored outline
  buffer.white_pen();

  canvas.copy_transparent_white(buffer, rc);

  // restore original color
  buffer.background_opaque();
}


void
MapWindow::DrawGlideThroughTerrain(Canvas &canvas)
{
  if (SettingsComputer().FinalGlideTerrain) {
    canvas.select(MapGfx.hpTerrainLineBg);
    canvas.polyline(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1);

    canvas.select(MapGfx.hpTerrainLine);
    canvas.polyline(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1);
  }

  if (!Basic().flight.Flying)
    return;

  if (!Calculated().TerrainWarningLocation.is_null()) {
    POINT sc;
    if (LonLat2ScreenIfVisible(Calculated().TerrainWarningLocation, &sc))
      MapGfx.hTerrainWarning.draw(canvas, get_bitmap_canvas(), sc.x, sc.y);
  }
}

