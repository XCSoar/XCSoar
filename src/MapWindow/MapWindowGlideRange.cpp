// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "MapCanvas.hpp"
#include "Look/MapLook.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoClip.hpp"
#include "Geo/GeoPoint.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "Route/FlatTriangleFanVisitor.hpp"
#include "util/StaticArray.hxx"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

#include <stdio.h>
#include <vector>

class TriangleCompound final : public FlatTriangleFanVisitor {
  /**
   * A copy of ReachFan::projection.
   */
  const FlatProjection flat_projection;

  /** Projection to use for GeoPoint -> PixelPoint conversion */
  const MapWindowProjection &proj;
  /** GeoClip instance used for TriangleFan clipping */
  const GeoClip clip;

  StaticArray<unsigned, FlatTriangleFanTree::MAX_FANS> sizes;

  /**
   * Concatenated geographic rings; ring i has sizes[i] vertices.
   */
  std::vector<GeoPoint> geo;

public:
  TriangleCompound(const FlatProjection &_flat_projection,
                   const MapWindowProjection &_proj) noexcept
    :flat_projection(_flat_projection), proj(_proj),
     clip(_proj.GetScreenBounds().Scale(1.1))
  {
  }

  bool empty() const noexcept {
    return sizes.empty();
  }

  auto size() const noexcept {
    return sizes.size();
  }

  void DrawFill(Canvas &canvas) const noexcept {
    MapCanvas map_canvas(canvas, proj, clip);
    unsigned start = 0;
    for (unsigned n : sizes) {
      map_canvas.FillPolygon(&geo[start], n);
      start += n;
    }
  }

  void DrawOutline(Canvas &canvas) const noexcept {
    MapCanvas map_canvas(canvas, proj, clip);
    unsigned start = 0;
    for (unsigned n : sizes) {
      map_canvas.DrawPolygonOutline(&geo[start], n);
      start += n;
    }
  }

  /* virtual methods from class FlatTriangleFanVisitor */

  void VisitFan([[maybe_unused]] FlatGeoPoint origin,
                std::span<const FlatGeoPoint> fan) noexcept override {

    if (fan.size() < 3 || sizes.full())
      return;

    GeoPoint g[ROUTEPOLAR_POINTS + 2];
    for (size_t i = 0; i < fan.size(); ++i)
      g[i] = flat_projection.Unproject(fan[i]);

    unsigned n = (unsigned)fan.size();
    if (g[0] == g[n - 1])
      n--;
    if (n < 3)
      return;

    GeoBounds bb(g[0]);
    for (unsigned i = 1; i < n; ++i)
      bb.Extend(g[i]);
    if (bb.IsValid() && !clip.Overlaps(bb))
      return;

    sizes.push_back(n);
    for (unsigned i = 0; i < n; ++i)
      geo.push_back(g[i]);
  }
};

void
MapWindow::DrawTerrainAbove(Canvas &canvas) noexcept
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
MapWindow::RenderTerrainAbove(Canvas &canvas, bool working) noexcept
{
  // Create a visitor for the Reach code
  TriangleCompound visitor(route_planner->GetTerrainReachProjection(),
                           render_projection);

  // Fill the TriangleCompound with all TriangleFans in range
  route_planner->AcceptInRange(render_projection.GetScreenBounds(),
                               visitor, working);

  // Exit early if not fans found
  if (visitor.empty())
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

    const GLEnable<GL_STENCIL_TEST> stencil_test;
    glClear(GL_STENCIL_BUFFER_BIT);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    canvas.SelectWhiteBrush();
    canvas.SelectNullPen();
    visitor.DrawFill(canvas);

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
    visitor.DrawOutline(buffer);

    // Select a white brush (will later be transparent)
    buffer.SelectNullPen();
    buffer.SelectWhiteBrush();

    // Draw the TerrainLine polygons to remove the
    // brush pattern from the polygon areas
    visitor.DrawFill(buffer);

    // Copy everything non-white to the buffer
    canvas.CopyTransparentWhite({0, 0}, render_projection.GetScreenSize(),
                                buffer, {0, 0});

    /* skip the separate terrain line step below, because we have done
       it already */
    return;

#endif

  }

  if (visitor.size() == 1) {
    /* only one fan: we can draw a simple polygon */

#ifdef ENABLE_OPENGL
    canvas.Select(reach_pen);
    visitor.DrawOutline(canvas);
#else
    // Select the TerrainLine pen
    canvas.SelectHollowBrush();
    canvas.Select(reach_pen);
    canvas.SetBackgroundOpaque();
    canvas.SetBackgroundColor(COLOR_WHITE);

    // drop out extraneous line from origin
#endif

    // Draw the TerrainLine polygon

#ifndef ENABLE_OPENGL
    visitor.DrawOutline(canvas);
#endif
  } else {
    /* more than one fan (turning reach enabled): we have to use a
       stencil to draw the outline, because the fans may overlap */

#ifdef ENABLE_OPENGL
  glEnable(GL_STENCIL_TEST);
  glClear(GL_STENCIL_BUFFER_BIT);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  glStencilFunc(GL_ALWAYS, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  canvas.SelectWhiteBrush();
  canvas.SelectNullPen();
  visitor.DrawFill(canvas);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_NOTEQUAL, 1, 1);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  canvas.Select(reach_pen_thick);
  visitor.DrawOutline(canvas);

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
  visitor.DrawOutline(buffer);

  // Select a white brush (will later be transparent)
  buffer.SelectNullPen();
  buffer.SelectWhiteBrush();

  // Draw the TerrainLine polygons again to remove
  // the lines connecting all the polygons
  //
  // This removes half of the TerrainLine line width !!
  visitor.DrawFill(buffer);

  // Copy everything non-white to the buffer
  canvas.CopyTransparentWhite({0, 0}, render_projection.GetScreenSize(),
                              buffer, {0, 0});

#endif
  }
}


void
MapWindow::DrawGlideThroughTerrain(Canvas &canvas) const noexcept
{
  if (!Calculated().flight.flying ||
      !Calculated().terrain_warning_location.IsValid() ||
      Calculated().terrain_warning_location.DistanceS(Basic().location) < 500)
    return;

  if (auto p = render_projection.GeoToScreenIfVisible(Calculated().terrain_warning_location))
    look.terrain_warning_icon.Draw(canvas, *p);
}

