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
#ifndef FLATPOINT_HPP
#define FLATPOINT_HPP

#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Compiler.h"

/**
 * 2-d Cartesian projected real-valued point
 */
struct FlatPoint 
{
  /** X location */
  fixed x;
  /** Y location */
  fixed y;

  /**
   * Constructor given known location
   *
   * @param _x X position
   * @param _y Y position
   *
   * @return Initialised object
   */
  gcc_constexpr_ctor
  FlatPoint(const fixed _x, const fixed _y): x(_x), y(_y) {}

  /**
   * Constructor at origin
   *
   * @return Initialised object
   */
  gcc_constexpr_ctor
  FlatPoint(): x(fixed_zero), y(fixed_zero) {}

  /**
   * Calculate cross product of two points
   *
   * @param p2 Other point
   *
   * @return Cross product
   */
  gcc_pure
  fixed CrossProduct(const FlatPoint &p2) const;

  /**
   * Multiply Y value of point
   *
   * @param a Value to multiply
   */
  void MultiplyY(const fixed a);

  /**
   * Subtract delta from this point
   *
   * @param p2 Point to subtract
   */
  void Subtract(const FlatPoint &p2);

  /**
   * Add delta to this point
   *
   * @param p2 Point to add
   */
  void Add(const FlatPoint &p2);

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
  fixed Distance(const FlatPoint &p) const;

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
  fixed Magnitude() const;

  /**
   * Test whether two points are co-located
   *
   * @param other Point to compare
   *
   * @return True if coincident
   */
  gcc_constexpr_method
  bool operator== (const FlatPoint other) const {
    return (x == other.x) && (y == other.y);
  }

  /**
   * Calculate dot product of one point with another
   *
   * @param other That point
   *
   * @return Dot product
   */
  fixed DotProduct(const FlatPoint &other) const {
    return x*other.x+y*other.y;
  }

  /**
   * Scale a point
   *
   * @param p Scale
   *
   * @return Scaled point
   */
  gcc_pure
  FlatPoint
  operator* (const fixed &p) const
  {
    FlatPoint res = *this;
    res.x *= p;
    res.y *= p;
    return res;
  }

  /**
   * Add one point to another
   *
   * @param p2 Point to add
   *
   * @return Added value
   */
  gcc_pure
  FlatPoint
  operator+ (const FlatPoint &p2) const
  {
    FlatPoint res = *this;
    res.x += p2.x;
    res.y += p2.y;
    return res;
  }

  /**
   * Add one point to self
   *
   * @param p2 Point to add
   *
   * @return Added value
   */
  FlatPoint
  operator+= (const FlatPoint &p2)
  {
    x += p2.x;
    y += p2.y;
    return *this;
  }

  /**
   * Subtract one point from another
   *
   * @param p2 Point to subtract
   *
   * @return Subtracted value
   */
  gcc_pure
  FlatPoint
  operator- (const FlatPoint &p2) const
  {
    FlatPoint res = *this;
    res.x -= p2.x;
    res.y -= p2.y;
    return res;
  }

  gcc_constexpr_method
  FlatPoint Half() const {
    return FlatPoint(::half(x), ::half(y));
  }
};

#endif
