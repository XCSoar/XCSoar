/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_LINE2D_HPP
#define XCSOAR_LINE2D_HPP

#include "Util.hpp"

class Angle;

/**
 * A line on a 2D plane described by two points.
 */
template<typename P>
struct Line2D {
  typedef P Point;
  typedef typename Point::scalar_type scalar_type;
  typedef typename Point::product_type product_type;

  Point a, b;

  constexpr Line2D(Point _a, Point _b):a(_a), b(_b) {}

  /**
   * Calculate squared length of line
   *
   * @return Squared length
   */
  gcc_pure
  product_type GetSquaredDistance() const {
    return Square<product_type>(b.x - a.x) + Square<product_type>(b.y - a.y);
  }

  constexpr Point GetMiddle() const {
    return (a + b) / 2;
  }

  constexpr Point Normal() const {
    return Point(a.y - b.y, b.x - a.x);
  }

  Line2D<P> &operator+=(P delta) {
    a += delta;
    b += delta;
    return *this;
  }

  Line2D<P> &operator-=(P delta) {
    a -= delta;
    b -= delta;
    return *this;
  }

  /**
   * Return dot product of two lines (vectors)
   */
  constexpr product_type DotProduct(Line2D<P> other) const {
    return (b - a).DotProduct(other.b - other.a);
  }

  constexpr product_type CrossProduct() const {
    return ::CrossProduct(a, b);
  }

  /**
   * Determine the point's location relative to the (infinite) line.
   *
   * @return positive value if #p is left of line, zero if it is on
   * the line, negative if it is right of the line
   */
  constexpr product_type LocatePoint(Point p) const {
    return ::CrossProduct(b - a, p - a);
  }
};

#endif
