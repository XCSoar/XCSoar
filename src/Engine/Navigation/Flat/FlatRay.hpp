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
#ifndef FLATRAY_HPP
#define FLATRAY_HPP

#include "FlatGeoPoint.hpp"
#include "Math/fixed.hpp"
#include <utility>
#include "Compiler.h"

/**
 * Projected ray (a point and vector) in 2-d cartesian integer coordinates
 */
class FlatRay {
public:
  const FlatGeoPoint point; /**< Origin of ray */
  const FlatGeoPoint vector; /**< Vector representing ray direction and length */
  const fixed fx; /**< speedups for box intersection test */
  const fixed fy; /**< speedups for box intersection test */

  /**
   * Constructor given start/end locations
   *
   * @param from Origin of ray
   * @param to End point of ray
   */
  FlatRay(const FlatGeoPoint& from,
          const FlatGeoPoint& to):
    point(from),vector(to-from),
    fx(vector.Longitude!=0? 1.0/vector.Longitude:0),
    fy(vector.Latitude!=0? 1.0/vector.Latitude:0) {};

  /**
   * Test whether two rays intersect
   *
   * @param that Other ray to test intersection with
   *
   * @return Parameter [0,1] of vector on this ray that intersection occurs (or -1 if fail)
   */
  gcc_pure
  fixed intersects(const FlatRay &that) const;

/** 
   * Parametric form of ray
   *
   * @param t Parameter [0,1] of ray
   *
   * @return Location of end point
   */
  gcc_pure
  FlatGeoPoint parametric(const fixed t) const;

  /**
   * Determine if two rays intersect away from their nodes
   */
  gcc_pure
  bool
  intersects_distinct(const FlatRay& that) const;

  // as above, but if true, also calculates t parameter
  bool
  intersects_distinct(const FlatRay& that, fixed& t) const;

private:
  gcc_pure
  std::pair<int, int> intersects_ratio(const FlatRay &that) const;
};

#endif
