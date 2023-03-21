// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PolarCoefficients
{
  double a, b, c;

  PolarCoefficients() noexcept = default;
  constexpr PolarCoefficients(double _a, double _b, double _c) noexcept
    :a(_a), b(_b), c(_c) {}

  /**
   * Construct an invalid object.
   */
  static constexpr PolarCoefficients Invalid() noexcept {
    return PolarCoefficients(0, 0, 0);
  }

  void SetInvalid() noexcept {
    a = b = c = 0;
  }

  [[gnu::pure]]
  bool IsValid() const noexcept;

  /**
   * Calculates the three polynomial polar coefficients from
   * three pairs of horizontal vs. vertical speed.
   */
  [[gnu::pure]]
  static PolarCoefficients From3VW(double v1, double v2, double v3,
                                   double w1, double w2, double w3) noexcept;

  /**
   * Calculates the three polynomial polar coefficients from
   * two pairs of horizontal vs. vertical speed. The first pair defines
   * the point where the polar is flat (derivative equals zero)!
   */
  [[gnu::pure]]
  static PolarCoefficients From2VW(double v1, double v2, double w1, double w2) noexcept;
};
