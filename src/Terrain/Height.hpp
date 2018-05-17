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

#ifndef XCSOAR_TERRAIN_HEIGHT_HPP
#define XCSOAR_TERRAIN_HEIGHT_HPP

#include <type_traits>

#include <stdint.h>
#include <math.h>

enum class TerrainType : uint8_t {
  UNKNOWN, GROUND, WATER
};

/**
 * A height value loaded from a GeoJPEG2000 / GeoTIFF image.  It is a
 * signed 16 bit integer with some special values.
 */
class TerrainHeight {
  /** invalid value for terrain */
  static constexpr int16_t INVALID = -32768;
  static constexpr int16_t WATER_THRESHOLD = -30000;

  int16_t value;

public:
  TerrainHeight() = default;
  explicit constexpr TerrainHeight(int16_t _value):value(_value) {}

  static constexpr TerrainHeight Invalid() {
    return TerrainHeight(INVALID);
  }

  constexpr bool IsInvalid() const {
    return value == INVALID;
  }

  constexpr bool IsWater() const {
    return value <= WATER_THRESHOLD && !IsInvalid();
  }

  constexpr bool IsSpecial() const {
    return value <= WATER_THRESHOLD;
  }

  constexpr TerrainType GetType() const {
    return !IsSpecial()
      ? TerrainType::GROUND
      : (IsWater()
         ? TerrainType::WATER
         : TerrainType::UNKNOWN);
  }

  constexpr int16_t GetValue() const {
    return value;
  }

  /**
   * Return the value, but replace "special" values with 0.  This is
   * used when we need some "valid" value (and not some "magic"
   * special value).  Sometimes, 0 is the best we can do.
   *
   * Use this function with care.  "0" is just a random value like any
   * other.  Don't use it for calculations where the altitude matters
   * (e.g. glide path calculations).
   */
  constexpr int16_t GetValueOr0() const {
    return !IsSpecial() ? value : 0;
  }

  /**
   * Convert this value to "double".  Water and invalid values are
   * converted to the given fallback values.
   */
  constexpr double ToDouble(double invalid_value,
                            double water_value=0.) const {
    return !IsSpecial()
      ? double(value)
      : (IsInvalid() ? invalid_value : water_value);
  }
};

static_assert(std::is_trivial<TerrainHeight>::value, "type is not trivial");

#endif
