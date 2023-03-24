// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * The identification number of a GliderLink traffic.
 */
class GliderLinkId {
  uint32_t value;

public:
  explicit constexpr
  GliderLinkId(uint32_t _value):value(_value) {}

  GliderLinkId() = default;

  constexpr
  bool operator==(GliderLinkId other) const {
    return value == other.value;
  }

  constexpr
  bool operator<(GliderLinkId other) const {
    return value < other.value;
  }
};
