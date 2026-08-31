// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/BulkPoint.hpp"
#include "Geo/GeoClip.hpp"
#include "Math/Point2D.hpp"
#include "util/AllocatedArray.hxx"

#include <cstdint>

class Canvas;
class Projection;
struct GeoPoint;
class SearchPointVector;

/**
 * A wrapper of #Canvas which draws to geographic coordinates
 * (latitude / longituded).
 */
class MapCanvas {
public:
  Canvas &canvas;
  const Projection &projection;
  const GeoClip clip;

  /**
   * A variable-length buffer for clipped GeoPoints.
   */
  AllocatedArray<GeoPoint> geo_points;

  /**
   * Cache a shape and draw it multiple times with prepare_*() and
   * draw_prepared().
   */
  AllocatedArray<BulkPixelPoint> raster_points;
  unsigned num_raster_points;

#ifdef ENABLE_OPENGL
  /**
   * Geographic-space ear-clip indices so pan does not rebuild the
   * fill from ClipPolygon vertices sliding along the view box.
   */
  AllocatedArray<FloatPoint2D> screen_points;
  AllocatedArray<uint16_t> triangle_indices;
  unsigned num_triangle_indices = 0;
#endif

public:
  MapCanvas(Canvas &_canvas, const Projection &_projection,
            const GeoClip &_clip) noexcept
    :canvas(_canvas), projection(_projection), clip(_clip) {}

  void DrawLine(GeoPoint a, GeoPoint b) noexcept;
  void DrawLineWithOffset(GeoPoint a, GeoPoint b) noexcept;
  void DrawCircle(const GeoPoint &center, double radius) noexcept;

  /**
   * Projects all points of the #SearchPointVector to screen
   * coordinates.
   *
   * @param screen a BulkPixelPoint array allocated by the caller, large enough
   * to hold all points of the #SearchPointVector
   */
  static void Project(const Projection &projection,
                      const SearchPointVector &points,
                      BulkPixelPoint *screen) noexcept;

  void Project(const SearchPointVector &points,
               BulkPixelPoint *screen) const noexcept {
    Project(projection, points, screen);
  }

  void DrawPolygon(const SearchPointVector &points) noexcept {
    FillPolygon(points);
  }

  void DrawPolygon(const GeoPoint *src, unsigned n) noexcept {
    if (PreparePolygon(src, n))
      DrawPrepared();
    DrawPolygonOutline(src, n);
  }

  void FillPolygon(const SearchPointVector &points) noexcept {
    if (PreparePolygon(points))
      DrawPrepared();
  }

  void FillPolygon(const GeoPoint *src, unsigned n) noexcept {
    if (PreparePolygon(src, n))
      DrawPrepared();
  }

  /**
   * Draw the border of a polygon.  Each edge is clipped as a line so
   * the view-box edges that ClipPolygon inserts for fills are not
   * drawn as a fake outline.
   */
  void DrawPolygonOutline(const SearchPointVector &points) noexcept;
  void DrawPolygonOutline(const GeoPoint *src, unsigned n) noexcept;

  /**
   * @return false if it's completely outside the screen (don't call
   * DrawPrepared())
   */
  bool PreparePolygon(const SearchPointVector &points) noexcept;
  bool PreparePolygon(const GeoPoint *src, unsigned n) noexcept;
  void DrawPrepared() noexcept;

private:
  bool PrepareCopied(unsigned n) noexcept;
};
