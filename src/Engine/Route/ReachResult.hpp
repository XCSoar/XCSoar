/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include <cstdint>
#include <stdlib.h>

/**
 * The result of a reach calculation.
 */
struct ReachResult {
  enum class Validity : uint8_t {
    INVALID,
    VALID,
    UNREACHABLE,
  };

  /**
   * The arrival altitude for straight glide, ignoring terrain
   * obstacles.
   */
  int direct;

  /**
   * The arrival altitude considering detour to avoid terrain
   * obstacles.  This attribute may only be used if #terrain_valid is
   * #VALID.
   */
  int terrain;

  /**
   * This attribute describes whether the #terrain attribute is valid.
   */
  Validity terrain_valid;

  constexpr void Clear() noexcept {
    terrain_valid = Validity::INVALID;
  }

  constexpr bool IsReachableDirect() const noexcept {
    return direct >= 0;
  }

  constexpr bool IsReachableTerrain() const noexcept {
    return terrain_valid == Validity::VALID && terrain >= 0;
  }

  constexpr bool IsDeltaConsiderable() const noexcept {
    if (terrain_valid != Validity::VALID)
      return false;

    const int delta = abs(direct - terrain);
    return delta >= 10 && delta * 100 / direct > 5;
  }

  constexpr bool IsReachRelevant() const noexcept {
    return terrain_valid == Validity::VALID && terrain != direct;
  }

  constexpr void Add(int delta) noexcept {
    direct += delta;
    terrain += delta;
  }

  constexpr void Subtract(int delta) noexcept {
    direct -= delta;
    terrain -= delta;
  }
};
