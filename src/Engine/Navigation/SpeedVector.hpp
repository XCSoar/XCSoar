/*
Copyright_License {

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

#ifndef XCSOAR_SPEED_VECTOR_HPP
#define XCSOAR_SPEED_VECTOR_HPP

#include "Math/Angle.hpp"
#include "Compiler.h"

/**
 * An object describing the speed vector in a two dimensional surface.
 */
struct SpeedVector {
  /**
   * The direction of the vector in degrees true (0..360).
   */
  Angle bearing;

  /**
   * The norm of the vector [m/s].
   */
  fixed norm;

  /** 
   * Constructor for null speed
   * 
   * @return Initialised object
   */
  SpeedVector():bearing(Angle::Zero()), norm(fixed_zero) {}

  /** 
   * Constructor given bearing and magnitude
   * 
   * @param _bearing Bearing of vector (degrees true)
   * @param _norm Magnitude of vector (m/s)
   * @return Initialised object
   */
  gcc_constexpr_ctor
  SpeedVector(Angle _bearing, fixed _norm):bearing(_bearing), norm(_norm) {}

  /** 
   * Constructor given two magnitudes (east and north)
   * 
   * @param _x East speed
   * @param _x North speed
   * @return Initialised object
   */
  SpeedVector(fixed x, fixed y):bearing(Angle::FromXY(y,x).AsBearing()), norm(sqrt(x*x+y*y)) {}

  /**
   * Returns true if the norm of the vector is zero.
   */
  bool is_zero() const {
    return !is_non_zero();
  }

  /**
   * Returns true if the norm of the vector is non-zero.
   */
  bool is_non_zero() const {
    return positive(norm);
  }

  /**
   * Return the vector with the bearing rotated by 180 degrees.
   */
  gcc_pure
  SpeedVector Reciprocal() const {
    return SpeedVector(bearing.Reciprocal(), norm);
  }
};

#endif
