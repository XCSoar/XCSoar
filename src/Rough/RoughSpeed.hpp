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

#ifndef XCSOAR_ROUGH_SPEED_HPP
#define XCSOAR_ROUGH_SPEED_HPP

#include <type_traits>

#include <stdint.h>

/**
 * Store an rough speed value, when the exact value is not needed.
 *
 * The accuracy is about 2mm/s. The range is 0 - 127 m/s.
 */
class RoughSpeed {
  uint16_t value;

  static constexpr uint16_t Import(double x) {
    return (uint16_t)(x * 512);
  }

  static constexpr double Export(uint16_t x) {
    return double(x) / 512;
  }

public:
  RoughSpeed() = default;
  RoughSpeed(double _value):value(Import(_value)) {}

  RoughSpeed &operator=(double other) {
    value = Import(other);
    return *this;
  }

  constexpr operator double() const {
    return Export(value);
  }
};

static_assert(std::is_trivial<RoughSpeed>::value, "type is not trivial");

#endif
