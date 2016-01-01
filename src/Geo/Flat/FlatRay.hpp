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
#ifndef FLATRAY_HPP
#define FLATRAY_HPP

#include "FlatGeoPoint.hpp"
#include "Compiler.h"

#include <utility>

/** Projected ray (a point and vector) in 2-d cartesian integer coordinates */
class FlatRay {
public:
  /** Origin of ray */
  FlatGeoPoint point;
  /** Vector representing ray direction and length */
  FlatGeoPoint vector;
  /** speedups for box intersection test */
  double fx;
  /** speedups for box intersection test */
  double fy;

  /**
   * Constructor given start/end locations
   *
   * @param from Origin of ray
   * @param to End point of ray
   */
  FlatRay(const FlatGeoPoint& from, const FlatGeoPoint& to)
    :point(from), vector(to - from),
     fx(vector.x != 0 ? 1.0 / vector.x : 0),
     fy(vector.y != 0 ? 1.0 / vector.y : 0) {}

  /**
   * Return the length of the ray.
   */
  gcc_pure
  int Magnitude() const;

  /**
   * Test whether two rays intersect
   *
   * @param that Other ray to test intersection with
   *
   * @return Parameter [0,1] of vector on this ray that intersection occurs (or -1 if fail)
   */
  gcc_pure
  double Intersects(const FlatRay &that) const;

  /**
   * Parametric form of ray
   *
   * @param t Parameter [0,1] of ray
   *
   * @return Location of end point
   */
  gcc_pure
  FlatGeoPoint Parametric(const double t) const;

  /**
   * Determine if two rays intersect away from their nodes
   */
  gcc_pure
  bool IntersectsDistinct(const FlatRay& that) const;

  /**
   * Determine if two rays intersect away from their nodes, and return
   * the "t" parameter.  Returns a negative number if the rays to not
   * intersect.
   */
  gcc_pure
  double DistinctIntersection(const FlatRay& that) const;

private:
  gcc_pure
  std::pair<int, int> IntersectsRatio(const FlatRay &that) const;
};

#endif
