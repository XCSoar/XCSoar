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

#ifndef XCSOAR_SPEED_VECTOR_HPP
#define XCSOAR_SPEED_VECTOR_HPP

#include "Math/Angle.hpp"
#include "Compiler.h"

/**
 * An object describing the speed vector in a two dimensional surface.
 */
struct SpeedVector {
  /**
   * The direction of the vector.
   */
  Angle bearing;

  /**
   * The norm of the vector [m/s].
   */
  double norm;

  /** 
   * Non-initialising default constructor.
   */
  SpeedVector() = default;

  /** 
   * Constructor given bearing and magnitude
   * 
   * @param _bearing the direction of vector
   * @param _norm Magnitude of vector (m/s)
   * @return Initialised object
   */
  constexpr
  SpeedVector(Angle _bearing, double _norm):bearing(_bearing), norm(_norm) {}

  /** 
   * Constructor given two magnitudes (east and north)
   * 
   * @param x East speed
   * @param y North speed
   * @return Initialised object
   */
  SpeedVector(double x, double y)
    :bearing(Angle::FromXY(y,x).AsBearing()), norm(hypot(x, y)) {}

  /**
   * Returns the null vector.
   */
  static constexpr SpeedVector Zero() {
    return SpeedVector(Angle::Zero(), 0);
  }

  /**
   * Returns true if the norm of the vector is zero.
   */
  constexpr bool IsZero() const {
    return !IsNonZero();
  }

  /**
   * Returns true if the norm of the vector is non-zero.
   */
  constexpr bool IsNonZero() const {
    return norm > 0;
  }

  /**
   * Return the vector with the bearing rotated by 180 degrees.
   */
  gcc_pure
  SpeedVector Reciprocal() const {
    return SpeedVector(bearing.Reciprocal(), norm);
  }
};

static_assert(std::is_trivial<SpeedVector>::value, "type is not trivial");

#endif
