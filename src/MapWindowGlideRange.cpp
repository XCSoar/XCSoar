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
#include "Task/ProtectedTaskManager.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Draw.hpp"
#endif

#include <stdio.h>

typedef std::vector<RasterPoint> RasterPointVector;

class TriangleCompound: public TriangleFanVisitor {
public:
  TriangleCompound(const MapWindowProjection& _proj)
    :proj(_proj),
     clip(_proj.GetScreenBounds().scale(fixed(1.1)))
  {
    g.reserve(50);
  }

  virtual void start_fan() {
    // Clear the GeoPointVector for the next TriangleFan
    g.clear();
  }

  virtual void add_point(const GeoPoint& p) {
    // Add a new GeoPoint to the current TriangleFan
    g.push_back(p);
  }

  virtual void
  end_fan()
  {
    // Perform clipping on the GeoPointVector (Result: clipped)
    unsigned size = clip.clip_polygon(clipped, &g[0], g.size());
    // With less than three points we can't draw a polygon
    if (size < 3)
      return;

    // Convert GeoPoints to RasterPoints
    RasterPointVector r;
    for (unsigned i = 0; i < size; ++i)
      r.push_back(proj.GeoToScreen(clipped[i]));

    // Save the RasterPoints in the fans vector
    fans.push_back(r);
  };

  /** STL-Container of rasterized polygons */
  std::vector<RasterPointVector> fans;

private:
  /** Temporary container for TriangleFan processing */
  std::vector<GeoPoint> g;
  /** Temporary container for TriangleFan clipping */
  GeoPoint clipped[50 * 4];
  /** Projection to use for GeoPoint -> RasterPoint conversion */
  const MapWindowProjection &proj;
  /** GeoClip instance used for TriangleFan clipping */
  const GeoClip clip;
};

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
  if (Basic().gps.NAVWarning
      || !Calculated().flight.Flying
      || !SettingsComputer().FinalGlideTerrain
      || !task)
    return;

  // Create a visitor for the Reach code
  TriangleCompound visitor(render_projection);

  // Fill the TriangleCompound with all TriangleFans in range
  task->accept_in_range(render_projection.GetScreenBounds(), visitor);

  // Exit early if not fans found
  if (visitor.fans.empty())
    return;

  {

#ifdef ENABLE_OPENGL
  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  canvas.null_pen();
  canvas.white_brush();
  for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
       i != visitor.fans.end(); ++i)
    canvas.polygon(&(*i)[0], i->size());

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_NOTEQUAL, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  canvas.select(Graphics::hpTerrainLine);
  for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
       i != visitor.fans.end(); ++i)
    canvas.polygon(&(*i)[0], i->size());

  glDisable(GL_STENCIL_TEST);

#elif !defined(ENABLE_SDL)

  Canvas &buffer = buffer_canvas;

  buffer.clear_white();

  buffer.hollow_brush();
  buffer.select(Graphics::hpTerrainLine);
  for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
       i != visitor.fans.end(); ++i)
    buffer.polygon(&(*i)[0], i->size());


  buffer.null_pen();
  buffer.white_brush();
  for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
       i != visitor.fans.end(); ++i)
    buffer.polygon(&(*i)[0], i->size());

  canvas.copy_transparent_white(buffer);

#endif
  }

  if ((SettingsComputer().FinalGlideTerrain != 2)
      || SettingsMap().EnablePan)
    return;

  // @todo: update this rendering

#ifdef ENABLE_OPENGL

  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  canvas.null_pen();
  canvas.white_brush();
  for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
       i != visitor.fans.end(); ++i)
    canvas.polygon(&(*i)[0], i->size());

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
  buffer.set_text_color(Color(0xf0, 0xf0, 0xf0));
  buffer.clear(Graphics::hAboveTerrainBrush);

  buffer.null_pen();
  buffer.white_brush();
  for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
       i != visitor.fans.end(); ++i)
    buffer.polygon(&(*i)[0], i->size());

  canvas.copy_transparent_white(buffer);

#endif
}


void
MapWindow::DrawGlideThroughTerrain(Canvas &canvas) const
{
  if (!Calculated().flight.Flying)
    return;

  if (Calculated().TerrainWarning) {
    RasterPoint sc;
    if (render_projection.GeoToScreenIfVisible(Calculated().TerrainWarningLocation,
                                                 sc))
      Graphics::hTerrainWarning.draw(canvas, sc.x, sc.y);
  }
}

