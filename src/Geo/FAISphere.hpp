/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
 * @brief Library for calculating Earth dimensions on the FAI sphere
 */

#ifndef XCSOAR_GEO_FAI_SPHERE_HPP
#define XCSOAR_GEO_FAI_SPHERE_HPP

#include "Math/Angle.hpp"
#include "util/Compiler.h"

namespace FAISphere {
  static constexpr unsigned REARTH = 6371000;

  /**
   * Convert a distance on earth's surface [m] to the according Angle,
   * assuming the earth is a sphere.
   */
  constexpr
  static inline Angle
  EarthDistanceToAngle(double distance)
  {
    return Angle::Radians(distance / REARTH);
  }

  /**
   * Convert an angle to the according distance on earth's surface [m],
   * assuming the earth is a sphere.
   */
  constexpr
  static inline double
  AngleToEarthDistance(Angle angle)
  {
    return angle.Radians() * REARTH;
  }
}

#endif
