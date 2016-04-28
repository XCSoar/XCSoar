/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Task/ProtectedRoutePlanner.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/VertexPointer.hpp"
#include "Screen/OpenGL/Triangulate.hpp"
#endif

#include <stdio.h>
#include "Util/StaticArray.hxx"

typedef std::vector<BulkPixelPoint> BulkPixelPointVector;

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
  void DrawFill(const BulkPixelPoint *points, unsigned start) const {
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
  void DrawFill(Canvas &canvas, const BulkPixelPoint *points) const {
    canvas.DrawPolygon(&points[0], size);
  }

  void DrawOutline(Canvas &canvas, const BulkPixelPoint *points) const {
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
  BulkPixelPointVector points;

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

  void Append(const PixelPoint &pt) {
#ifndef NDEBUG
    assert(remaining > 0);
    --remaining;
#endif

    points.push_back(pt);
  }

  void DrawFill(Canvas &canvas) const {
    assert(remaining == 0);

#ifdef ENABLE_OPENGL
    unsigned start = 0;
    const auto *points = &this->points[0];
    for (auto i = fans.begin(), end = fans.end(); i != end; ++i) {
      i->DrawFill(points, start);
      start += i->size;
    }
#else
    const auto *points = &this->points[0];
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
    const auto *points = &this->points[0];
    for (auto i = fans.begin(), end = fans.end(); i != end; ++i) {
      i->DrawOutline(canvas, points);
      points += i->size;
    }
#endif
  }
};

typedef StaticArray<ProjectedFan, FlatTriangleFanTree::REACH_MAX_FANS> ProjectedFanVector;

class TriangleCompound final : public FlatTriangleFanVisitor {
  /**
   * A copy of ReachFan::projection.
   */
  const FlatProjection flat_projection;

  /** Projection to use for GeoPoint -> PixelPoint conversion */
  const MapWindowProjection &proj;
  /** GeoClip instance used for TriangleFan clipping */
  const GeoClip clip;

public:
  /** STL-Container of rasterized polygons */
  ProjectedFans fans;

  TriangleCompound(const FlatProjection &_flat_projection,
                   const MapWindowProjection& _proj)
    :flat_projection(_flat_projection), proj(_proj),
     clip(_proj.GetScreenBounds().Scale(1.1))
  {
  }

  /* virtual methods from class FlatTriangleFanVisitor */

  void VisitFan(FlatGeoPoint origin, ConstBuffer<FlatGeoPoint> fan) override {

    if (fan.size < 3 || fans.full())
      return;

    GeoPoint g[ROUTEPOLAR_POINTS + 2];
    for (size_t i = 0; i < fan.size; ++i)
      g[i] = flat_projection.Unproject(fan[i]);

    // Perform clipping on the GeoPointVector
    GeoPoint clipped[(ROUTEPOLAR_POINTS + 2) * 3];
    unsigned size = clip.ClipPolygon(clipped, g, fan.size);
    // With less than three points we can't draw a polygon
    if (size < 3)
      return;

    // Work directly on the PixelPoints in the fans vector
    fans.Append(size);

    // Convert GeoPoints to PixelPoints
    for (unsigned i = 0; i < size; ++i)
      fans.Append(proj.GeoToScreen(clipped[i]));
  }
};

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
      || route_planner == nullptr)
    return;

  if ((GetComputerSettings().features.final_glide_terrain == FeaturesSettings::FinalGlideTerrain::WORKING) ||
      (GetComputerSettings().features.final_glide_terrain == FeaturesSettings::FinalGlideTerrain::WORKING_TERRAIN_LINE) ||
      (GetComputerSettings().features.final_glide_terrain == FeaturesSettings::FinalGlideTerrain::WORKING_TERRAIN_SHADE)) {
    RenderTerrainAbove(canvas, true);
  }

  if ((GetComputerSettings().features.final_glide_terrain != FeaturesSettings::FinalGlideTerrain::OFF) &&
      (GetComputerSettings().features.final_glide_terrain != FeaturesSettings::FinalGlideTerrain::WORKING)) {
    RenderTerrainAbove(canvas, false);
  }
}

/**
 * Draw the final glide groundline (and shading) to the buffer
 * and copy the transparent buffer to the canvas
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 * @param buffer The drawing buffer
 */
void
MapWindow::RenderTerrainAbove(Canvas &canvas, bool working)
{
  // Create a visitor for the Reach code
  TriangleCompound visitor(route_planner->GetTerrainReachProjection(),
                           render_projection);

  // Fill the TriangleCompound with all TriangleFans in range
  {
    const ProtectedRoutePlanner::Lease lease(*route_planner);
    lease->AcceptInRange(render_projection.GetScreenBounds(), visitor, working);
  }

  // Exit early if not fans found
  if (visitor.fans.empty())
    return;

  const Pen& reach_pen = working? look.reach_working_pen : look.reach_terrain_pen;
  const Pen& reach_pen_thick = working? look.reach_working_pen_thick : look.reach_terrain_pen_thick;
  // @todo: update this rendering

  // Don't draw shade if
  // .. shade feature disabled
  // .. pan mode activated
  // .. working reach (rather than terrain reach)
  if (IsNearSelf() && !working &&
      ((GetComputerSettings().features.final_glide_terrain == FeaturesSettings::FinalGlideTerrain::TERRAIN_SHADE) ||
       (GetComputerSettings().features.final_glide_terrain == FeaturesSettings::FinalGlideTerrain::WORKING_TERRAIN_SHADE))) {

#ifdef ENABLE_OPENGL

    const ScopeVertexPointer vp(&visitor.fans.points[0]);

    const GLEnable<GL_STENCIL_TEST> stencil_test;
    glClear(GL_STENCIL_BUFFER_BIT);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    COLOR_WHITE.Bind();
    visitor.fans.DrawFill(canvas);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_NOTEQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    const ScopeAlphaBlend alpha_blend;

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
    buffer.Select(reach_pen_thick);
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
    canvas.CopyTransparentWhite(0, 0,
                                render_projection.GetScreenWidth(),
                                render_projection.GetScreenHeight(),
                                buffer, 0, 0);

    /* skip the separate terrain line step below, because we have done
       it already */
    return;

#endif

  }

  if (visitor.fans.size() == 1) {
    /* only one fan: we can draw a simple polygon */

#ifdef ENABLE_OPENGL
    const ScopeVertexPointer vp(&visitor.fans.points[0]);
    reach_pen.Bind();
#else
    // Select the TerrainLine pen
    canvas.SelectHollowBrush();
    canvas.Select(reach_pen);
    canvas.SetBackgroundOpaque();
    canvas.SetBackgroundColor(COLOR_WHITE);

    // drop out extraneous line from origin
#endif

    // Draw the TerrainLine polygon

    visitor.fans.DrawOutline(canvas);

#ifdef ENABLE_OPENGL
    reach_pen.Unbind();
#endif
  } else {
    /* more than one fan (turning reach enabled): we have to use a
       stencil to draw the outline, because the fans may overlap */

#ifdef ENABLE_OPENGL
  const ScopeVertexPointer vp(&visitor.fans.points[0]);

  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  COLOR_WHITE.Bind();
  visitor.fans.DrawFill(canvas);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_NOTEQUAL, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  reach_pen_thick.Bind();
  visitor.fans.DrawOutline(canvas);
  reach_pen_thick.Unbind();

  glDisable(GL_STENCIL_TEST);

#elif defined(USE_GDI) || defined(USE_MEMORY_CANVAS)

  // Get a buffer for drawing a mask
  Canvas &buffer = buffer_canvas;

  // Paint the whole buffer canvas white ( = transparent)
  buffer.ClearWhite();

  // Select the TerrainLine pen
  buffer.SelectHollowBrush();
  buffer.Select(reach_pen_thick);
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
  canvas.CopyTransparentWhite(0, 0,
                              render_projection.GetScreenWidth(),
                              render_projection.GetScreenHeight(),
                              buffer, 0, 0);

#endif
  }
}


void
MapWindow::DrawGlideThroughTerrain(Canvas &canvas) const
{
  if (!Calculated().flight.flying ||
      !Calculated().terrain_warning_location.IsValid() ||
      Calculated().terrain_warning_location.DistanceS(Basic().location) < 500)
    return;

  PixelPoint sc;
  if (render_projection.GeoToScreenIfVisible(Calculated().terrain_warning_location,
                                             sc))
    look.terrain_warning_icon.Draw(canvas, sc);
}

