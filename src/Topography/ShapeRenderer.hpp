// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "util/NonCopyable.hpp"
#include "util/AllocatedArray.hxx"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Brush.hpp"

#include "util/MapBenchToggles.hpp"

#include <algorithm>
#include <cassert>
#include <string_view>

/**
 * Minimum screen-space spacing (pixels) between polyline vertices.
 * Used by #AddPointIfDistant on the software renderer.
 */
constexpr int SHAPE_MIN_POINT_SPACING_PX = 8;

/**
 * Drop a whole fill only if its bbox is smaller than this on screen.
 * Must not be used for polylines: at 120 km, even 1 px is ~150 m and
 * skipping short OSM road sticks leaves a broken network.
 */
constexpr int SHAPE_MIN_BBOX_PX = 3;

/**
 * Fill vertex spacing and skip-bbox, indexed by
 * #TopographyFile::GetFillThinningLevel.  Independent of the layer's
 * range so water and city polygons still thin at a 30–50 km view.
 */
constexpr int SHAPE_FILL_SPACING_PX[] = {2, 4, 8, 16};
constexpr int SHAPE_FILL_MIN_BBOX_PX[] = {3, 6, 12, 24};
/** City / building / forest blobs: skip sooner than water. */
constexpr int SHAPE_LANDCOVER_SPACING_PX[] = {4, 8, 16, 24};
constexpr int SHAPE_LANDCOVER_MIN_BBOX_PX[] = {8, 16, 32, 64};
/**
 * Polyline vertex spacing (roads, rail, rivers).  Indexed by fill
 * thinning level.  Whole sticks shorter than #SHAPE_LINE_MIN_BBOX_PX
 * are dropped separately (two-point OSM splits cannot be spaced).
 */
constexpr int SHAPE_LINE_SPACING_PX[] = {2, 4, 6, 10};
/**
 * Skip a whole polyline if its bbox is smaller than this.  0 means
 * never skip.  Drops OSM ramps and junction splits that are only
 * two points (vertex spacing cannot thin those).  Keep this small
 * so the main carriageway does not get holes.
 */
constexpr int SHAPE_LINE_MIN_BBOX_PX[] = {0, 1, 2, 3};
/**
 * Merge a parallel polyline (dual carriageway) when the two
 * midpoints are closer than this on screen.  0 keeps both.  Zoomed
 * in they separate in pixels and both are drawn again.
 */
constexpr int SHAPE_LINE_COLLAPSE_PX[] = {0, 2, 3, 4};
static_assert(sizeof(SHAPE_FILL_SPACING_PX) /
              sizeof(SHAPE_FILL_SPACING_PX[0]) == 4);
static_assert(sizeof(SHAPE_FILL_MIN_BBOX_PX) /
              sizeof(SHAPE_FILL_MIN_BBOX_PX[0]) == 4);
static_assert(sizeof(SHAPE_LANDCOVER_SPACING_PX) /
              sizeof(SHAPE_LANDCOVER_SPACING_PX[0]) == 4);
static_assert(sizeof(SHAPE_LANDCOVER_MIN_BBOX_PX) /
              sizeof(SHAPE_LANDCOVER_MIN_BBOX_PX[0]) == 4);
static_assert(sizeof(SHAPE_LINE_SPACING_PX) /
              sizeof(SHAPE_LINE_SPACING_PX[0]) == 4);
static_assert(sizeof(SHAPE_LINE_MIN_BBOX_PX) /
              sizeof(SHAPE_LINE_MIN_BBOX_PX[0]) == 4);
static_assert(sizeof(SHAPE_LINE_COLLAPSE_PX) /
              sizeof(SHAPE_LINE_COLLAPSE_PX[0]) == 4);

[[gnu::pure]]
inline bool
IsDenseLandcoverLayer(std::string_view name) noexcept
{
  return name.starts_with("city_area") ||
    name.starts_with("building_area") ||
    name.starts_with("forest_area");
}

[[gnu::pure]]
inline unsigned
FillSpacingPx(unsigned fill_level, const char *layer) noexcept
{
  if (MapBenchTopoHead() || MapBenchTopoHas("nofill"))
    return 2;
  fill_level = std::min(fill_level, 3u);
  return unsigned(IsDenseLandcoverLayer(layer)
                  ? SHAPE_LANDCOVER_SPACING_PX[fill_level]
                  : SHAPE_FILL_SPACING_PX[fill_level]);
}

[[gnu::pure]]
inline int
FillMinBBoxPx(unsigned fill_level, const char *layer) noexcept
{
  if (MapBenchTopoHead())
    return 1;
  if (MapBenchTopoHas("nofill"))
    return SHAPE_MIN_BBOX_PX;
  fill_level = std::min(fill_level, 3u);
  return IsDenseLandcoverLayer(layer)
    ? SHAPE_LANDCOVER_MIN_BBOX_PX[fill_level]
    : SHAPE_FILL_MIN_BBOX_PX[fill_level];
}

[[gnu::pure]]
inline int
LineMinBBoxPx(unsigned fill_level) noexcept
{
  if (MapBenchTopoHead())
    return 1;
  if (MapBenchTopoHas("noline"))
    return 0;
  fill_level = std::min(fill_level, 3u);
  return SHAPE_LINE_MIN_BBOX_PX[fill_level];
}

[[gnu::pure]]
inline bool
IsDualCarriagewayLayer(std::string_view name) noexcept
{
  return name.starts_with("road") || name.starts_with("railway");
}

[[gnu::pure]]
inline int
LineCollapsePx(unsigned fill_level, const char *layer) noexcept
{
  if (MapBenchTopoHead() || MapBenchTopoHas("nocollapse"))
    return 0;
  if (!IsDualCarriagewayLayer(layer))
    return 0;
  fill_level = std::min(fill_level, 3u);
  return SHAPE_LINE_COLLAPSE_PX[fill_level];
}

/**
 * A helper class optimized for doing bulk draws on OpenGL.
 */
class ShapeRenderer : private NonCopyable {
  AllocatedArray<BulkPixelPoint> points;
  unsigned num_points;

  const Pen *pen;
  const Brush *brush;

  enum { NONE, OUTLINE, SOLID } mode;

public:
  void Configure(const Pen *_pen, const Brush *_brush) {
    pen = _pen;
    brush = _brush;
    mode = NONE;

    num_points = 0;
  }

  void Begin(unsigned n) {
    assert(num_points == 0);

    points.GrowDiscard(((n - 1) | 0x3ff) + 1);
  }

  void AddPoint(PixelPoint pt) {
    assert(num_points < points.size());

    points[num_points++] = pt;
  }

  /**
   * Adds the point only if it a few pixels distant from the previous
   * one.  Useful to reduce the complexity of small figures.
   */
   void AddPointIfDistant(PixelPoint pt) {
    assert(num_points < points.size());

    if (num_points == 0 ||
        ManhattanDistance((PixelPoint)points[num_points - 1], pt) >=
          SHAPE_MIN_POINT_SPACING_PX)
      AddPoint(pt);
  }

  void FinishPolyline(Canvas &canvas) {
    if (mode != OUTLINE) {
      canvas.Select(*pen);
      mode = OUTLINE;
    }

    canvas.DrawPolyline(points.data(), num_points);

    num_points = 0;
  }

  void FinishPolygon(Canvas &canvas) {
    if (mode != SOLID) {
      canvas.SelectNullPen();
      canvas.Select(*brush);
      mode = SOLID;
    }

    canvas.DrawPolygon(points.data(), num_points);

    num_points = 0;
  }

  void Commit() {
    assert(num_points == 0);
  }
};
