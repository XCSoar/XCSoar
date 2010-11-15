/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Navigation/SearchPointVector.hpp"
#include "Screen/Point.hpp"

class Canvas;
class Projection;
struct GeoPoint;

/**
 * A wrapper of #Canvas which draws to geographic coordinates
 * (latitude / longituded).
 */
class MapCanvas {
public:
  Canvas &canvas;
  const Projection &projection;

public:
  MapCanvas(Canvas &_canvas, const Projection &_projection)
    :canvas(_canvas), projection(_projection) {}

  void line(const GeoPoint &a, const GeoPoint &b);
  void circle(const GeoPoint &center, fixed radius);

  /**
   * Projects all points of the #SearchPointVector to screen
   * coordinates.
   *
   * @param screen a RasterPoint array allocated by the caller, large enough
   * to hold all points of the #SearchPointVector
   */
  void project(const SearchPointVector &points, RasterPoint *screen) const;

  /**
   * Determines whether the polygon is visible, or off-screen.
   *
   * Calling Canvas::polygon() is a very expensive operation on
   * Windows CE, even if no single pixel of the polygon is visible,
   * and this function aims to reduce the overhead for off-screen
   * airspaces.
   */
  bool visible(const RasterPoint *screen, unsigned num);

  void draw(const SearchPointVector &points);
};

#endif
