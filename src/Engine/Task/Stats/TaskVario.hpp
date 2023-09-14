// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <type_traits>

/**
 * Helper class to produce pseudo variometer based on rate of change
 * of task altitude difference.
 */
class TaskVario
{
  friend class TaskVarioComputer;

  double value;

public:
  constexpr void Reset() noexcept {
    value = 0;
  }

/** 
 * Retrieve current vario value from last update
 * 
 * @return Current vario value (m/s, positive up)
 */
  constexpr double get_value() const noexcept {
    return value;
  }
};

static_assert(std::is_trivial<TaskVario>::value, "type is not trivial");
