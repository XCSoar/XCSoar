/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Look/MapLook.hpp"
#include "Geo/GeoClip.hpp"
#include "Screen/Icon.hpp"
#include "Task/ProtectedRoutePlanner.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Triangulate.hpp"
#endif

#include <stdio.h>
#include "Util/StaticArray.hpp"

typedef std::vector<RasterPoint> RasterPointVector;

struct ProjectedFan {
  /**
   * The number of points of the associated ReachFan.  The first
   * ProjectedFan starts of ProjectedFans::points[0], followed by the
   * second one at ProjectedFans::points[reach_fan0.size], etc.
   */
  unsigned size;

  ProjectedFan() = default;

  ProjectedFan(unsigned n):size(n) {
  }

#ifdef ENABLE_OPENGL
  void DrawFill(const RasterPoint *points, unsigned start) const {
    /* triangulate the polygon */
    AllocatedArray<GLushort> triangle_buffer;

    unsigned idx_count = PolygonToTriangles(points + start, size,
                                            triangle_buffer);
    if (idx_count == 0)
      return;

    /* add offset to all vertex indices */
    for (unsigned i = 0; i < idx_count; ++i)
      triangle_buffer[i] += start;

    glDrawElements(GL_TRIANGLES, idx_count, GL_UNSIGNED_SHORT,
                   triangle_buffer.begin());
  }

  void DrawOutline(unsigned start) const {
    glDrawArrays(GL_LINE_LOOP, start, size);
  }
#else
  void DrawFill(Canvas &canvas, const RasterPoint *points) const {
    canvas.DrawPolygon(&points[0], size);
  }

  void DrawOutline(Canvas &canvas, const RasterPoint *points) const {
    canvas.DrawPolygon(&points[0], size);
  }
#endif
};

struct ProjectedFans {
  typedef StaticArray<ProjectedFan, FlatTriangleFanTree::REACH_MAX_FANS> ProjectedFanVector;

  ProjectedFanVector fans;

  /**
   * All points of all ProjectedFan objects.  The first one starts of
   * points[0], followed by the second one at points[fans[0].size],
   * etc.
   */
  RasterPointVector points;

#ifndef NDEBUG
  unsigned remaining;
#endif

  ProjectedFans()
#ifndef NDEBUG
    :remaining(0)
#endif
  {
    /* try to guess the total number of vertices */
    points.reserve(FlatTriangleFanTree::REACH_MAX_FANS * ROUTEPOLAR_POINTS / 10);
  }

  bool empty() const {
    return fans.empty();
  }

  bool full() const {
    return fans.full();
  }

  ProjectedFanVector::size_type size() const {
    return fans.size();
  }

  ProjectedFan &Append(unsigned n) {
#ifndef NDEBUG
    assert(remaining == 0);
    remaining = n;
#endif

    points.reserve(points.size() + n);

    fans.push_back(ProjectedFan(n));
    return fans.back();
  }

  void Append(const RasterPoint &pt) {
#ifndef NDEBUG
    assert(remaining > 0);
    --remaining;
#endif

    points.push_back(pt);
  }

#ifdef ENABLE_OPENGL
  void Prepare() {
    glVertexPointer(2, GL_VALUE, 0, &points[0]);
  }
#endif

  void DrawFill(Canvas &canvas) const {
    assert(remaining == 0);

#ifdef ENABLE_OPENGL
    unsigned start = 0;
    const RasterPoint *points = &this->points[0];
    for (auto i = fans.begin(), end = fans.end(); i != end; ++i) {
      i->DrawFill(points, start);
      start += i->size;
    }
#else
    const RasterPoint *points = &this->points[0];
    for (auto i = fans.begin(), end = fans.end(); i != end; ++i) {
      i->DrawFill(canvas, points);
      points += i->size;
    }
#endif
  }

  void DrawOutline(Canvas &canvas) const {
    assert(remaining == 0);

#ifdef ENABLE_OPENGL
    unsigned start = 0;
    for (auto i = fans.begin(), end = fans.end(); i != end; ++i) {
      i->DrawOutline(start);
      start += i->size;
    }
#else
    const RasterPoint *points = &this->points[0];
    for (auto i = fans.begin(), end = fans.end(); i != end; ++i) {
      i->DrawOutline(canvas, points);
      points += i->size;
    }
#endif
  }
};

typedef StaticArray<ProjectedFan, FlatTriangleFanTree::REACH_MAX_FANS> ProjectedFanVector;

class TriangleCompound: public TriangleFanVisitor {
  /** Temporary container for TriangleFan processing */
  StaticArray<GeoPoint, ROUTEPOLAR_POINTS+2> g;
  /** Temporary container for TriangleFan clipping */
  GeoPoint clipped[(ROUTEPOLAR_POINTS+2) * 3];
  /** Projection to use for GeoPoint -> RasterPoint conversion */
  const MapWindowProjection &proj;
  /** GeoClip instance used for TriangleFan clipping */
  const GeoClip clip;

public:
  /** STL-Container of rasterized polygons */
  ProjectedFans fans;

  TriangleCompound(const MapWindowProjection& _proj)
    :proj(_proj),
     clip(_proj.GetScreenBounds().Scale(fixed(1.1)))
  {
  }

  virtual void StartFan() {
    // Clear the GeoPointVector for the next TriangleFan
    g.clear();
  }

  virtual void AddPoint(const GeoPoint& p) {
    // Add a new GeoPoint to the current TriangleFan
    g.append(p);
  }

  virtual void
  EndFan()
  {
    if (fans.full())
      return;

    // remove unnecessary inclusion of origin if next and last points are identical
    unsigned start = 0;
    const size_t gsize = g.size();
    if (gsize > 2 && g[gsize - 1] == g[1])
      start = 1;

    if (gsize < start + 3)
      return;

    // Perform clipping on the GeoPointVector (Result: clipped)
    unsigned size = clip.ClipPolygon(clipped, g.raw() + start, gsize - start);
    // With less than three points we can't draw a polygon
    if (size < 3)
      return;

    // Work directly on the RasterPoints in the fans vector
    fans.Append(size);

    // Convert GeoPoints to RasterPoints
    for (unsigned i = 0; i < size; ++i)
      fans.Append(proj.GeoToScreen(clipped[i]));
  }
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
  if (!Basic().location_available
      || !Calculated().flight.flying
      || GetComputerSettings().features.final_glide_terrain == FeaturesSettings::FinalGlideTerrain::OFF
      || route_planner == NULL)
    return;

  // Create a visitor for the Reach code
  TriangleCompound visitor(render_projection);

  // Fill the TriangleCompound with all TriangleFans in range
  route_planner->AcceptInRange(render_projection.GetScreenBounds(), visitor);

  // Exit early if not fans found
  if (visitor.fans.empty())
    return;

  // @todo: update this rendering

  // Don't draw shade if
  // .. shade feature disabled
  // .. pan mode activated
  if (GetComputerSettings().features.final_glide_terrain == FeaturesSettings::FinalGlideTerrain::SHADE &&
      IsNearSelf()) {

#ifdef ENABLE_OPENGL

    visitor.fans.Prepare();

    const GLEnable stencil_test(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    COLOR_WHITE.Set();
    visitor.fans.DrawFill(canvas);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_NOTEQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    const GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    canvas.Clear(Color(255, 255, 255, 77));

#elif defined(USE_GDI)

    // Get a buffer for drawing a mask
    Canvas &buffer = buffer_canvas;

    // Set the pattern colors
    buffer.SetBackgroundOpaque();
    buffer.SetBackgroundColor(COLOR_WHITE);
    buffer.SetTextColor(Color(0xd0, 0xd0, 0xd0));

    // Paint the whole buffer canvas with a pattern brush (small dots)
    buffer.Clear(look.above_terrain_brush);

    // Select the TerrainLine pen
    buffer.SelectHollowBrush();
    buffer.Select(look.reach_pen_thick);
    buffer.SetBackgroundColor(Color(0xf0, 0xf0, 0xf0));

    // Draw the TerrainLine polygons
    visitor.fans.DrawOutline(buffer);

    // Select a white brush (will later be transparent)
    buffer.SelectNullPen();
    buffer.SelectWhiteBrush();

    // Draw the TerrainLine polygons to remove the
    // brush pattern from the polygon areas
    visitor.fans.DrawFill(buffer);

    // Copy everything non-white to the buffer
    canvas.CopyTransparentWhite(buffer);

    /* skip the separate terrain line step below, because we have done
       it already */
    return;

#endif

  }

  if (visitor.fans.size() == 1) {
    /* only one fan: we can draw a simple polygon */

#ifdef ENABLE_OPENGL
    visitor.fans.Prepare();
    look.reach_pen.Bind();
#else
    // Select the TerrainLine pen
    canvas.SelectHollowBrush();
    canvas.Select(look.reach_pen);
    canvas.SetBackgroundOpaque();
    canvas.SetBackgroundColor(COLOR_WHITE);

    // drop out extraneous line from origin
#endif

    // Draw the TerrainLine polygon

    visitor.fans.DrawOutline(canvas);

#ifdef ENABLE_OPENGL
    look.reach_pen.Unbind();
#endif
  } else {
    /* more than one fan (turning reach enabled): we have to use a
       stencil to draw the outline, because the fans may overlap */

#ifdef ENABLE_OPENGL
  visitor.fans.Prepare();

  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  COLOR_WHITE.Set();
  visitor.fans.DrawFill(canvas);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_NOTEQUAL, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  look.reach_pen_thick.Bind();
  visitor.fans.DrawOutline(canvas);
  look.reach_pen_thick.Unbind();

  glDisable(GL_STENCIL_TEST);

#elif defined(USE_GDI) || defined(USE_MEMORY_CANVAS)

  // Get a buffer for drawing a mask
  Canvas &buffer = buffer_canvas;

  // Paint the whole buffer canvas white ( = transparent)
  buffer.ClearWhite();

  // Select the TerrainLine pen
  buffer.SelectHollowBrush();
  buffer.Select(look.reach_pen_thick);
  buffer.SetBackgroundOpaque();
  buffer.SetBackgroundColor(Color(0xf0, 0xf0, 0xf0));

  // Draw the TerrainLine polygons
  visitor.fans.DrawOutline(buffer);

  // Select a white brush (will later be transparent)
  buffer.SelectNullPen();
  buffer.SelectWhiteBrush();

  // Draw the TerrainLine polygons again to remove
  // the lines connecting all the polygons
  //
  // This removes half of the TerrainLine line width !!
  visitor.fans.DrawFill(buffer);

  // Copy everything non-white to the buffer
  canvas.CopyTransparentWhite(buffer);

#endif
  }
}


void
MapWindow::DrawGlideThroughTerrain(Canvas &canvas) const
{
  if (!Calculated().flight.flying ||
      !Calculated().terrain_warning ||
      Calculated().terrain_warning_location.Distance(Basic().location) < fixed(500.0))
    return;

  RasterPoint sc;
  if (render_projection.GeoToScreenIfVisible(Calculated().terrain_warning_location,
                                             sc))
    look.terrain_warning_icon.Draw(canvas, sc.x, sc.y);
}

