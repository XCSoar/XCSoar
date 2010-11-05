/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Screen/Icon.hpp"

#include <stdio.h>

void MapWindow::CalculateScreenPositionsGroundline(void) {
  if (SettingsComputer().FinalGlideTerrain)
    render_projection.GeoToScreen(Calculated().GlideFootPrint,
                                    Groundline,
                                    TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1);
}

/**
 * Draw the final glide groundline (and shading) to the buffer
 * and copy the transparent buffer to the canvas
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 * @param buffer The drawing buffer
 */
void
MapWindow::DrawTerrainAbove(Canvas &canvas)
{
  if (!Basic().flight.Flying)
    return;

  Canvas &buffer = buffer_canvas;

  buffer.set_background_color(Color::WHITE);
  buffer.set_text_color(Color(0xf0,0xf0,0xf0));
  buffer.clear(Graphics::hAboveTerrainBrush);

  buffer.white_pen();
  buffer.white_brush();
  buffer.polygon(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1);

  canvas.copy_transparent_white(buffer);
}


void
MapWindow::DrawGlideThroughTerrain(Canvas &canvas) const
{
  if (SettingsComputer().FinalGlideTerrain) {
    canvas.select(Graphics::hpTerrainLineBg);
    canvas.polyline(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1);

    canvas.select(Graphics::hpTerrainLine);
    canvas.polyline(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1);
  }

  if (!Basic().flight.Flying)
    return;

  if (!Calculated().TerrainWarningLocation.is_null()) {
    POINT sc;
    if (render_projection.GeoToScreenIfVisible(Calculated().TerrainWarningLocation,
                                                 sc))
      Graphics::hTerrainWarning.draw(canvas, bitmap_canvas, sc.x, sc.y);
  }
}

