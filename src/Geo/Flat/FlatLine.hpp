/* Copyright_License {

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

#ifndef FLATLINE_HPP
#define FLATLINE_HPP

#include "FlatPoint.hpp"
#include "Math/Line2D.hpp"
#include "Compiler.h"

/**
 * Defines an infinite line in real-valued cartesian coordinates,
 * with intersection methods.
 */
class FlatLine : public Line2D<FlatPoint>
{
  typedef Line2D<FlatPoint> Base;

public:
  /**
   * Constructor given known start/end points
   *
   * @param _p1 Start of line
   * @param _p2 End of line
   *
   * @return Initialised object
   */
  constexpr
  FlatLine(const FlatPoint _p1, const FlatPoint _p2):Base(_p1, _p2) {}

  /** 
   * Calculate intersections between this line
   * and a circle of specified radius centered at the origin.
   * 
   * @param r Radius of circle
   * @param i1 Returned intersection point 1
   * @param i2 Returned intersection point 2
   * 
   * @return True if more than one intersection is found
   */
  bool IntersectOriginCircle(double r, FlatPoint &i1, FlatPoint &i2) const;

  /** 
   * Calculate intersections between this line
   * and a circle of specified radius centered at point c.
   * 
   * @param r Radius of circle
   * @param c Center of circle
   * @param i1 Returned intersection point 1
   * @param i2 Returned intersection point 2
   * 
   * @return True if more than one intersection is found
   */
  bool IntersectCircle(double r, FlatPoint c,
                       FlatPoint &i1, FlatPoint &i2) const;

  using Base::GetMiddle;

  /**
   * Find angle of this line starting from the x-axis counter-clockwise
   *
   * @return Angle (deg)
   */
  gcc_pure
  Angle GetAngle() const;

  using Base::GetSquaredDistance;

  /**
   * Calculate length of line
   *
   * @return Length
   */
  gcc_pure
  double GetDistance() const {
    return a.Distance(b);
  }

  constexpr FlatLine operator+(FlatPoint delta) const {
    return {a + delta, b + delta};
  }

  constexpr FlatLine operator-(FlatPoint delta) const {
    return {a - delta, b - delta};
  }

  /**
   * Rotate line clockwise around origin
   *
   * @param angle Angle (deg) to rotate line clockwise
   */
  void Rotate(const Angle angle);

  /**
   * Scale line in Y direction
   */
  void MultiplyY(const double factor) {
    a.MultiplyY(factor);
    b.MultiplyY(factor);
  }

  using Base::DotProduct;
};

#endif
