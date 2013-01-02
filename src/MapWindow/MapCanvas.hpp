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

#ifndef XCSOAR_MAP_CANVAS_HPP
#define XCSOAR_MAP_CANVAS_HPP

#include "Screen/Point.hpp"
#include "Geo/GeoClip.hpp"
#include "Util/AllocatedArray.hpp"

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
  AllocatedArray<RasterPoint> raster_points;
  unsigned num_raster_points;
  unsigned screen_radius;

public:
  MapCanvas(Canvas &_canvas, const Projection &_projection,
            const GeoClip &_clip)
    :canvas(_canvas), projection(_projection), clip(_clip) {}

  void DrawLine(GeoPoint a, GeoPoint b);
  void DrawLineWithOffset(GeoPoint a, GeoPoint b);
  void DrawCircle(const GeoPoint &center, fixed radius);

  /**
   * Projects all points of the #SearchPointVector to screen
   * coordinates.
   *
   * @param screen a RasterPoint array allocated by the caller, large enough
   * to hold all points of the #SearchPointVector
   */
  static void Project(const Projection &projection,
                      const SearchPointVector &points,
                      RasterPoint *screen);

  void Project(const SearchPointVector &points, RasterPoint *screen) const {
    Project(projection, points, screen);
  }

  /**
   * Determines whether the polygon is visible, or off-screen.
   *
   * Calling Canvas::DrawPolygon() is a very expensive operation on
   * Windows CE, even if no single pixel of the polygon is visible,
   * and this function aims to reduce the overhead for off-screen
   * airspaces.
   */
  gcc_pure
  static bool IsVisible(const Canvas &canvas,
                      const RasterPoint *screen, unsigned num);

  gcc_pure
  bool IsVisible(const RasterPoint *screen, unsigned num) const {
    return IsVisible(canvas, screen, num);
  }

  void DrawPolygon(const SearchPointVector &points) {
    if (PreparePolygon(points))
      DrawPrepared();
  }

  bool PreparePolygon(const SearchPointVector &points);
  void DrawPrepared();
};

#endif
