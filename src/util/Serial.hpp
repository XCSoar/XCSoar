// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * A serial version number for an object.  It can be used to check
 * whether an object has been modified.  Useful to Invalidate cached
 * values.
 */
class Serial {
  unsigned value = 0;

public:
  constexpr Serial &operator++() noexcept {
    ++value;
    return *this;
  }

  constexpr bool operator==(const Serial other) const noexcept {
    return value == other.value;
  }

  constexpr bool operator!=(const Serial other) const noexcept {
    return value != other.value;
  }
};
