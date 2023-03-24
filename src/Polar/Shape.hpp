// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>

struct PolarCoefficients;

struct PolarPoint {
  /**
   * Speed of point [m/s].
   */
  double v;

  /**
   * Sink rate of point [m/s].  Must be negative.
   */
  double w;
};

struct PolarShape {
  std::array<PolarPoint, 3> points;
  double reference_mass; /**< Reference Mass (kg) */

  const PolarPoint &operator[](unsigned i) const noexcept {
    return points[i];
  }

  PolarPoint &operator[](unsigned i) noexcept {
    return points[i];
  }

  [[gnu::pure]]
  PolarCoefficients CalculateCoefficients() const noexcept;

  [[gnu::pure]]
  bool IsValid() const noexcept;
};
