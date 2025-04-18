// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <compare> // for the defaulted spaceship operator
#include <cstdint>

/**
 * A reference to a trace/search point: first element is the stage
 * number (turn point number); second element is the index in the
 * #TracePointVector / #SearchPointVector.
 */
class ScanTaskPoint {
  uint32_t value;

public:
  constexpr
  ScanTaskPoint(unsigned stage_number, unsigned point_index) noexcept
    :value((stage_number << 16) | point_index) {}

  /**
   * Generate a unique key that is used for the std::map comparison
   * operator.
   */
  constexpr
  uint32_t Key() const noexcept {
    return value;
  }

  friend constexpr auto operator<=>(const ScanTaskPoint &,
                                    const ScanTaskPoint &) noexcept = default;

  constexpr
  unsigned GetStageNumber() const noexcept {
    return value >> 16;
  }

  constexpr
  unsigned GetPointIndex() const noexcept {
    return value & 0xffff;
  }

  void SetPointIndex(unsigned i) noexcept {
    value = (value & 0xffff0000) | i;
  }

  void IncrementPointIndex() noexcept {
    ++value;
  }

  /**
   * Determine whether a point is a starting point (no previous edges).
   */
  constexpr
  bool IsFirst() const noexcept {
    return GetStageNumber() == 0;
  }
};
