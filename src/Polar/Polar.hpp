// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Shape.hpp"

struct PolarCoefficients;

/**
 * Struct for internally stored WinPilot-like polars
 */
struct PolarInfo
{
  // Using doubles here to simplify the code in PolarStore.cpp
  //
  double max_ballast;  /**< Max water ballast (l) */

  PolarShape shape;

  double wing_area;    /**< Reference wing area (m^2) */
  double v_no;         /**< Maximum speed for normal operations (m/s) */

  [[gnu::pure]]
  PolarCoefficients CalculateCoefficients() const noexcept;

  bool IsValid() const noexcept {
    return shape.IsValid();
  }
};
