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
                                    TERRAIN_ALT_INFO::NUMTERRAINSWEEPS);
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
  if (!Basic().flight.Flying || SettingsMap().EnablePan)
    return;

#ifdef ENABLE_OPENGL
  glClearStencil(0);
  glClear(GL_STENCIL_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  canvas.polygon(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_NOTEQUAL, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(1.0, 1.0, 1.0, 0.3);
  const GLfloat v[] = {
    0, 0,
    canvas.get_width(), 0,
    canvas.get_width(), canvas.get_height(),
    0, canvas.get_height(),
  };
  glVertexPointer(2, GL_FLOAT, 0, v);

#ifdef ANDROID
  GLubyte i[] = { 0, 1, 2, 0, 2, 3 };
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, i);
#else
  glDrawArrays(GL_QUADS, 0, 4);
#endif

  glDisable(GL_BLEND);
  glDisable(GL_STENCIL_TEST);
#else
  Canvas &buffer = buffer_canvas;

  buffer.set_background_color(Color::WHITE);
  buffer.set_text_color(Color(0xf0,0xf0,0xf0));
  buffer.clear(Graphics::hAboveTerrainBrush);

  buffer.null_pen();
  buffer.white_brush();
  buffer.polygon(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS);

  canvas.copy_transparent_white(buffer);
#endif
}


void
MapWindow::DrawGlideThroughTerrain(Canvas &canvas) const
{
  if (SettingsComputer().FinalGlideTerrain) {
    canvas.hollow_brush();

    canvas.select(Graphics::hpTerrainLineBg);
    canvas.polygon(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS);

    canvas.select(Graphics::hpTerrainLine);
    canvas.polygon(Groundline, TERRAIN_ALT_INFO::NUMTERRAINSWEEPS);
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

