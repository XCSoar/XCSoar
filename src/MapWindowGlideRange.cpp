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

#include <stdio.h>
#include "Util/StaticArray.hpp"

typedef std::vector<RasterPoint> RasterPointVector;

class TriangleCompound: public TriangleFanVisitor {
public:
  TriangleCompound(const MapWindowProjection& _proj)
    :proj(_proj),
     clip(_proj.GetScreenBounds().scale(fixed(1.1)))
  {
  }

  virtual void allocate_fans(const unsigned size) {
    fans.reserve(size);
  }

  virtual void start_fan() {
    // Clear the GeoPointVector for the next TriangleFan
    g.clear();
  }

  virtual void add_point(const GeoPoint& p) {
    // Add a new GeoPoint to the current TriangleFan
    g.append(p);
  }

  virtual void
  end_fan()
  {
    // Perform clipping on the GeoPointVector (Result: clipped)
    unsigned size = clip.clip_polygon(clipped, g.raw(), g.size());
    // With less than three points we can't draw a polygon
    if (size < 3)
      return;

    // Work directly on the RasterPoints in the fans vector
    fans.push_back(RasterPointVector());
    std::vector<RasterPointVector>::reverse_iterator it = fans.rbegin();

    // Convert GeoPoints to RasterPoints
    for (unsigned i = 0; i < size; ++i)
      it->push_back(proj.GeoToScreen(clipped[i]));
  };

  /** STL-Container of rasterized polygons */
  std::vector<RasterPointVector> fans;

private:
  /** Temporary container for TriangleFan processing */
  StaticArray<GeoPoint, ROUTEPOLAR_POINTS+2> g;
  /** Temporary container for TriangleFan clipping */
  GeoPoint clipped[(ROUTEPOLAR_POINTS+2) * 3];
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
  // Don't draw at all if
  // .. no GPS fix
  // .. not flying
  // .. feature disabled
  // .. feature inaccessible
  if (!Basic().LocationAvailable
      || !Calculated().flight.Flying
      || SettingsComputer().FinalGlideTerrain == SETTINGS_COMPUTER::FGT_OFF
      || !task)
    return;

  // Create a visitor for the Reach code
  TriangleCompound visitor(render_projection);

  // Fill the TriangleCompound with all TriangleFans in range
  task->accept_in_range(render_projection.GetScreenBounds(), visitor);

  // Exit early if not fans found
  if (visitor.fans.empty())
    return;

  // @todo: update this rendering

  // Don't draw shade if
  // .. shade feature disabled
  // .. pan mode activated
  if (SettingsComputer().FinalGlideTerrain == SETTINGS_COMPUTER::FGT_SHADE &&
      !SettingsMap().EnablePan) {

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

    canvas.clear(Color(255, 255, 255, 77));

    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);

#elif !defined(ENABLE_SDL)

    // Get a buffer for drawing a mask
    Canvas &buffer = buffer_canvas;

    // Set the pattern colors
    buffer.background_opaque();
    buffer.set_background_color(COLOR_WHITE);
    buffer.set_text_color(Color(0xf0, 0xf0, 0xf0));

    // Paint the whole buffer canvas with a pattern brush (small dots)
    buffer.clear(Graphics::hAboveTerrainBrush);

    // Select the TerrainLine pen
    buffer.hollow_brush();
    buffer.select(Graphics::hpTerrainLineThick);
    buffer.set_background_color(Color(0xf0, 0xf0, 0xf0));

    // Draw the TerrainLine polygons
    for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
         i != visitor.fans.end(); ++i)
      buffer.polygon(&(*i)[0], i->size());

    // Select a white brush (will later be transparent)
    buffer.null_pen();
    buffer.white_brush();

    // Draw the TerrainLine polygons to remove the
    // brush pattern from the polygon areas
    for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
         i != visitor.fans.end(); ++i)
      buffer.polygon(&(*i)[0], i->size());

    // Copy everything non-white to the buffer
    canvas.copy_transparent_white(buffer);

    /* skip the separate terrain line step below, because we have done
       it already */
    return;

#endif

  }

  if (visitor.fans.size() == 1) {
    /* only one fan: we can draw a simple polygon */

    // Select the TerrainLine pen
    canvas.hollow_brush();
    canvas.select(Graphics::hpTerrainLine);
    canvas.background_opaque();
    canvas.set_background_color(COLOR_WHITE);

    // Draw the TerrainLine polygon

    const RasterPointVector &fan = visitor.fans.front();
    canvas.polygon(&fan[0], fan.size());
  } else {
    /* more than one fan (turning reach enabled): we have to use a
       stencil to draw the outline, because the fans may overlap */

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

  canvas.select(Graphics::hpTerrainLineThick);
  for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
       i != visitor.fans.end(); ++i)
    canvas.polygon(&(*i)[0], i->size());

  glDisable(GL_STENCIL_TEST);

#elif !defined(ENABLE_SDL)

  // Get a buffer for drawing a mask
  Canvas &buffer = buffer_canvas;

  // Paint the whole buffer canvas white ( = transparent)
  buffer.clear_white();

  // Select the TerrainLine pen
  buffer.hollow_brush();
  buffer.select(Graphics::hpTerrainLineThick);
  buffer.background_opaque();
  buffer.set_background_color(Color(0xf0, 0xf0, 0xf0));

  // Draw the TerrainLine polygons
  for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
       i != visitor.fans.end(); ++i)
    buffer.polygon(&(*i)[0], i->size());

  // Select a white brush (will later be transparent)
  buffer.null_pen();
  buffer.white_brush();

  // Draw the TerrainLine polygons again to remove
  // the lines connecting all the polygons
  //
  // This removes half of the TerrainLine line width !!
  for (std::vector<RasterPointVector>::const_iterator i = visitor.fans.begin();
       i != visitor.fans.end(); ++i)
    buffer.polygon(&(*i)[0], i->size());

  // Copy everything non-white to the buffer
  canvas.copy_transparent_white(buffer);

#endif
  }
}


void
MapWindow::DrawGlideThroughTerrain(Canvas &canvas) const
{
  if (!Calculated().flight.Flying)
    return;

  if (!Calculated().TerrainWarning)
    return;

  if (Calculated().TerrainWarningLocation.distance(Basic().Location) < fixed(500.0))
    return;

  RasterPoint sc;
  if (render_projection.GeoToScreenIfVisible(Calculated().TerrainWarningLocation,
                                             sc))
    Graphics::hTerrainWarning.draw(canvas, sc.x, sc.y);
}

