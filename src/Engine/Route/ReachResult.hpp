// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
