/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Compiler.h"

/**
 * Defines an infinite line in real-valued cartesian coordinates,
 * with intersection methods.
 */
class FlatLine 
{
  FlatPoint p1;
  FlatPoint p2;

public:
  /**
   * Constructor given known start/end points
   *
   * @param _p1 Start of line
   * @param _p2 End of line
   *
   * @return Initialised object
   */
  gcc_constexpr_ctor
  FlatLine(const FlatPoint _p1, const FlatPoint _p2):p1(_p1),p2(_p2) {}

  /**
   * Constructor default
   *
   * @return Initialised object at origin
   */
  FlatLine() {}

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
  bool intersect_czero(const fixed r, FlatPoint &i1, FlatPoint &i2) const;

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
  bool intersect_circle(const fixed r, const FlatPoint c,
                        FlatPoint &i1, FlatPoint &i2) const;

  /**
   * Find center point of this line
   *
   * @return Center point
   */
  gcc_pure
  FlatPoint ave() const;

  /**
   * Find angle of this line starting from the x-axis counter-clockwise
   *
   * @return Angle (deg)
   */
  gcc_pure
  Angle angle() const;

  /**
   * Calculate squared length of line
   *
   * @return Squared length
   */
  gcc_pure
  fixed dsq() const;

  /**
   * Calculate length of line
   *
   * @return Length
   */
  gcc_pure
  fixed d() const;

  /**
   * Subtract a delta from the line (both start and end points)
   *
   * @param p Point to subtract
   */
  void sub(const FlatPoint&p);

  /**
   * Add a delta to the line (both start and end points)
   *
   * @param p Point to add
   */
  void add(const FlatPoint&p);

  /**
   * Rotate line clockwise around origin
   *
   * @param angle Angle (deg) to rotate line clockwise
   */
  void rotate(const Angle angle);

  /**
   * Scale line in Y direction
   *
   * @param a Scale ratio
   */
  void mul_y(const fixed a);

  /**
   * Return dot product of two lines (vectors)
   * @param that other line to take dot product of
   * @return Dot product
   */
  gcc_pure
  fixed dot(const FlatLine& that) const;

private:
  gcc_pure
  fixed dx() const;

  gcc_pure
  fixed dy() const;

  gcc_pure
  fixed cross() const;
};

#endif
