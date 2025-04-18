// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <cassert>
#include <type_traits>

/**
 * Derived climb rate history
 * 
 */
class ClimbHistory {
  /**
   * Store vario history from 0 to 360 kph.
   */
  static constexpr std::size_t SIZE = 100;

  /** Average climb rate for each episode */
  std::array<double, SIZE> vario;

  /** Number of samples in each episode */
  std::array<unsigned short, SIZE> count;

public:
  void Clear();

  void Add(unsigned speed, double vario);

  /**
   * Do we have data for the specified speed?
   */
  bool Check(unsigned speed) const {
    return speed < count.size() && count[speed] > 0;
  }

  /**
   * Returns the average climb rate for the specified speed.
   */
  double Get(unsigned speed) const {
    assert(speed < vario.size());
    assert(count[speed] > 0);

    return vario[speed] / count[speed];
  }
};

static_assert(std::is_trivial<ClimbHistory>::value, "type is not trivial");
