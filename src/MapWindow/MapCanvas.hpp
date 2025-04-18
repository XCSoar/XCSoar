// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/BulkPoint.hpp"
#include "Geo/GeoClip.hpp"
#include "util/AllocatedArray.hxx"

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
    if (PreparePolygon(points))
      DrawPrepared();
  }

  /**
   * @return false if it's completely outside the screen (don't call
   * DrawPrepared())
   */
  bool PreparePolygon(const SearchPointVector &points) noexcept;
  void DrawPrepared() noexcept;
};
