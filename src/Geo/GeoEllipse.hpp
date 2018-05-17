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

#ifndef GEOELLIPSE_HPP
#define GEOELLIPSE_HPP

#include "Flat/FlatProjection.hpp"
#include "Flat/FlatEllipse.hpp"
#include "Compiler.h"

/**
 * Ellipse in geodesic coordinates, defined by two foci and
 * a point on the ellipse.  Internally uses a flat-earth projection
 * to avoid complex and expensive geodesic calculations.
 */
class GeoEllipse
{
  FlatProjection projection;
  FlatEllipse ell;

public:
  /**
   * Constructor
   *
   * @param f1 Focus 1
   * @param f2 Focus 2
   * @param p Point on ellipse
   * @param _projection projection used for internal representation
   */
  GeoEllipse(const GeoPoint &f1, const GeoPoint &f2,
             const GeoPoint &p, const FlatProjection &_projection);

  /**
   * Parametric form of ellipse border
   *
   * @param t Parameter (0,1)
   *
   * @return Location of point on ellipse
   */
  gcc_pure
  GeoPoint Parametric(double t) const;

  /**
   * Calculate where a line from the first focus through a point p
   * intersects with the ellipse.
   *
   * @param p Origin of point from which to search
   * @param i1 Location of closest intersection point
   * @param i2 Location of furthest intersection point
   *
   * @return True if line intersects
   */
  bool IntersectExtended(const GeoPoint &p, GeoPoint &i1, GeoPoint &i2) const;
};


#endif
