// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"

/**
 * A point structure to be used in arrays.
 */
struct BulkPixelPoint : PixelPoint {
  BulkPixelPoint() = default;

  template<typename... Args>
  constexpr BulkPixelPoint(Args&&... args) noexcept
    :PixelPoint(args...) {}
};
