/*
Copyright_License {

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

/*! @file
 * @brief Library for calculating Earth dimensions
 *
 * This library provides general functions for calculating dimensions
 * on the Earth with GPS coordinates.  The implementations are
 * "simplified", i.e. they are fast but not as accurate as they could
 * be.  Instead of using the WGS84 model, they assume the FAI sphere.
 */

#ifndef XCSOAR_GEO_SIMPLIFIED_MATH_HPP
#define XCSOAR_GEO_SIMPLIFIED_MATH_HPP

#include "Compiler.h"

struct GeoPoint;
class Angle;

/**
 * Calculates the distance and bearing of two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @param Distance Pointer to the distance variable
 * @param Bearing Pointer to the bearing variable
 */
void
DistanceBearingS(const GeoPoint &loc1, const GeoPoint &loc2,
                 Angle *distance, Angle *bearing);

void
DistanceBearingS(const GeoPoint &loc1, const GeoPoint &loc2,
                 double *distance, Angle *bearing);

/**
 * @see FindLatitudeLongitude()
 */
gcc_pure
GeoPoint
FindLatitudeLongitudeS(const GeoPoint &loc, Angle bearing, double distance);

/**
 * @see ProjectedDistance()
 */
gcc_pure
double
ProjectedDistanceS(const GeoPoint &loc1, const GeoPoint &loc2,
                   const GeoPoint &loc3);

#endif
