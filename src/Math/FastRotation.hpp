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

#ifndef XCSOAR_MATH_FASTROTATION_HPP
#define XCSOAR_MATH_FASTROTATION_HPP

#include "Compiler.h"
#include "Math/Angle.hpp"

#include <utility>

/**
 * Rotate coordinates around the zero origin.
 */
class FastRotation {
  Angle angle;
  fixed cost, sint;

public:
  typedef std::pair<fixed,fixed> Pair;

  FastRotation()
    :angle(Angle::Zero()), cost(1), sint(0) {}
  FastRotation(Angle _angle):angle(Angle::Radians(-fixed(9999))) { SetAngle(_angle); }

  Angle GetAngle() const {
    return angle;
  }

  /**
   * Sets the new angle, and precalculates the sine/cosine values.
   *
   * @param _angle an angle between 0 and 360
   */
  void SetAngle(Angle _angle);

  const FastRotation &operator =(Angle _angle) {
    SetAngle(_angle);
    return *this;
  }

  /**
   * Rotates the point (xin, yin).
   *
   * @param x X value
   * @param y Y value
   * @return the rotated coordinates
   */
  gcc_pure
  Pair Rotate(fixed x, fixed y) const;

  gcc_pure
  Pair Rotate(const Pair p) const {
    return Rotate(p.first, p.second);
  }
};

/**
 * Same as #FastRotation, but works with integer coordinates.
 */
class FastIntegerRotation {
  Angle angle;
  int cost, sint;

  friend class FastRowRotation;

public:
  typedef std::pair<int,int> Pair;

  FastIntegerRotation()
 :angle(Angle::Zero()), cost(1024), sint(0) {}
  FastIntegerRotation(Angle _angle):angle(Angle::Radians(-fixed(9999))) { SetAngle(_angle); }

  Angle GetAngle() const {
    return angle;
  }

  void SetAngle(Angle _angle);

  const FastIntegerRotation &operator =(Angle _angle) {
    SetAngle(_angle);
    return *this;
  }

  /**
   * Rotates the point (xin, yin).
   *
   * @param x X value
   * @param y Y value
   * @return the rotated coordinates
   */
  gcc_pure
  Pair Rotate(int x, int y) const;

  gcc_pure
  Pair Rotate(const Pair p) const {
    return Rotate(p.first, p.second);
  }
};

/**
 * Similar to FastIntegerRotation, but supports scanning one screen
 * row (y is constant).
 */
class FastRowRotation {
  const int cost, sint, y_cost, y_sint;

public:
  typedef FastIntegerRotation::Pair Pair;

  FastRowRotation(const FastIntegerRotation &fir, int y)
    :cost(fir.cost), sint(fir.sint),
     y_cost(y * cost + 512), y_sint(y * sint - 512) {}

  gcc_pure
  Pair Rotate(int x) const {
    return Pair((x * cost - y_sint + 512) >> 10,
                (y_cost + x * sint + 512) >> 10);
  }
};

#endif
