/* Copyright_License {

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
#ifndef FLATPOINT_HPP
#define FLATPOINT_HPP

#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Math/Point2D.hpp"
#include "Compiler.h"

/**
 * 2-d Cartesian projected real-valued point
 */
struct FlatPoint : Point2D<fixed>
{
  /**
   * Non-initialising default constructor.
   */
  FlatPoint() = default;

  /**
   * Constructor given known location
   *
   * @param _x X position
   * @param _y Y position
   *
   * @return Initialised object
   */
  constexpr
  FlatPoint(const fixed _x, const fixed _y):Point2D<fixed>(_x, _y) {}

  /**
   * Calculate cross product of two points
   *
   * @param p2 Other point
   *
   * @return Cross product
   */
  gcc_pure
  fixed CrossProduct(const FlatPoint &p2) const {
    return ::CrossProduct(*this, p2);
  }

  /**
   * Multiply Y value of point
   *
   * @param a Value to multiply
   */
  void MultiplyY(const fixed a) {
    y *= a;
  }

  /**
   * Rotate point counter-clockwise around origin
   *
   * @param angle Angle (deg) to rotate point counter-clockwise
   */
  void Rotate(const Angle angle);

  /**
   * Calculate distance between two points
   *
   * @param p Other point
   *
   * @return Distance
   */
  gcc_pure
  fixed Distance(FlatPoint p) const {
    return (*this - p).Magnitude();
  }

  /**
   * Find dx * dx + dy * dy
   * @return Magnitude squared
   */
  gcc_pure
  fixed MagnitudeSquared() const;

  /**
   * Find sqrt(dx * dx + dy * dy)
   * @return Magnitude
   */
  gcc_pure
  fixed Magnitude() const {
    return hypot(x, y);
  }

  /**
   * Calculate dot product of one point with another
   *
   * @param other That point
   *
   * @return Dot product
   */
  gcc_pure
  fixed DotProduct(FlatPoint other) const {
    return ::DotProduct(*this, other);
  }

  /**
   * Scale a point
   *
   * @param p Scale
   *
   * @return Scaled point
   */
  gcc_pure
  FlatPoint operator*(fixed p) const
  {
    return { x * p, y * p };
  }

  /**
   * Add one point to another
   *
   * @param p2 Point to add
   *
   * @return Added value
   */
  constexpr FlatPoint operator+(FlatPoint other) const
  {
    return { x + other.x, y + other.y };
  }

  /**
   * Subtract one point from another
   *
   * @param p2 Point to subtract
   *
   * @return Subtracted value
   */
  constexpr FlatPoint operator-(const FlatPoint other) const
  {
    return { x - other.x, y - other.y };
  }

  constexpr
  FlatPoint Half() const {
    return FlatPoint(::Half(x), ::Half(y));
  }
};

#endif
