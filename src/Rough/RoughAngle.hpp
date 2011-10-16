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

#ifndef XCSOAR_ROUGH_ANGLE_HPP
#define XCSOAR_ROUGH_ANGLE_HPP

#include "Math/fixed.hpp"
#include "Compiler.h"

#include <stdint.h>

/**
 * Store an rough angle, when the exact value is not needed.
 *
 * The accuracy is about 0.02 degrees.
 */
class RoughAngle {
  int16_t value;

  gcc_const
  static int16_t Import(Angle x) {
    return (int16_t)(x.Radians() * 4096);
  }

  gcc_const
  static Angle Export(int16_t x) {
    return Angle::Radians(fixed(x) / 4096);
  }

  gcc_constexpr_ctor
  RoughAngle(int16_t _value):value(_value) {}

public:
  RoughAngle() {}
  RoughAngle(Angle _value):value(Import(_value)) {}

  RoughAngle &operator=(Angle other) {
    value = Import(other);
    return *this;
  }

  operator Angle() const {
    return Export(value);
  }

  gcc_constexpr_method
  RoughAngle operator-(RoughAngle other) const {
    return RoughAngle(value - other.value);
  }
};

#endif
