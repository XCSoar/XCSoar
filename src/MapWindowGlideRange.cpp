/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Geo/GeoClip.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Icon.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Draw.hpp"
#endif

#include <stdio.h>

void MapWindow::CalculateScreenPositionsGroundline(void)
{
  if (!SettingsComputer().FinalGlideTerrain)
    return;

  const GeoClip clip(render_projection.GetScreenBounds().scale(fixed(1.1)));
  GeoPoint clipped[TERRAIN_ALT_INFO::NUMTERRAINSWEEPS * 3];
  GroundlineLength =
    clip.clip_polygon(clipped, Calculated().GlideFootPrint,
                      TERRAIN_ALT_INFO::NUMTERRAINSWEEPS);
  if (GroundlineLength < 3)
    return;

  for (unsigned i = 0; i < GroundlineLength; ++i)
    Groundline[i] = render_projection.GeoToScreen(clipped[i]);
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
  if (Basic().gps.NAVWarning || !Basic().flight.Flying ||
      SettingsMap().EnablePan || GroundlineLength < 3)
    return;

#ifdef ENABLE_OPENGL
  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  canvas.null_pen();
  canvas.white_brush();
  canvas.polygon(Groundline, GroundlineLength);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_NOTEQUAL, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(1.0, 1.0, 1.0, 0.3);
  GLFillRectangle(0, 0, canvas.get_width(), canvas.get_height());

  glDisable(GL_BLEND);
  glDisable(GL_STENCIL_TEST);
#elif !defined(ENABLE_SDL)
  Canvas &buffer = buffer_canvas;

  buffer.set_background_color(Color::WHITE);
  buffer.set_text_color(Color(0xf0,0xf0,0xf0));
  buffer.clear(Graphics::hAboveTerrainBrush);

  buffer.null_pen();
  buffer.white_brush();
  buffer.polygon(Groundline, GroundlineLength);

  canvas.copy_transparent_white(buffer);
#endif
}


void
MapWindow::DrawGlideThroughTerrain(Canvas &canvas) const
{
  if (Basic().gps.NAVWarning)
    /* don't draw this if we don't know our own position */
    return;

  if (SettingsComputer().FinalGlideTerrain && GroundlineLength >= 3) {
    canvas.hollow_brush();
    canvas.background_opaque();
    canvas.set_background_color(Color::WHITE);
    canvas.select(Graphics::hpTerrainLine);
    canvas.polygon(Groundline, GroundlineLength);
  }

  if (!Basic().flight.Flying)
    return;

  if (!Calculated().TerrainWarningLocation.is_null()) {
    RasterPoint sc;
    if (render_projection.GeoToScreenIfVisible(Calculated().TerrainWarningLocation,
                                                 sc))
      Graphics::hTerrainWarning.draw(canvas, sc.x, sc.y);
  }
}

