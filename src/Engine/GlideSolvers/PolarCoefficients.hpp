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

  constexpr void SetInvalid() noexcept {
    a = b = c = 0;
  }

  [[gnu::pure]]
  constexpr bool IsValid() const noexcept {
    return a > 0 && b < 0 && c > 0;
  }

  /**
   * Calculates the three polynomial polar coefficients from
   * three pairs of horizontal vs. vertical speed.
   */
  [[gnu::pure]]
  static constexpr PolarCoefficients From3VW(double v1, double v2, double v3,
                                             double w1, double w2, double w3) noexcept {
    PolarCoefficients pc;

    auto d = v1 * v1 * (v2 - v3) + v2 * v2 * (v3 - v1) + v3 * v3 * (v1 - v2);
    pc.a = d == 0
      ? 0.
      : -((v2 - v3) * (w1 - w3) + (v3 - v1) * (w2 - w3)) / d;

    d = v2 - v3;
    pc.b = d == 0
      ? 0.
      : -(w2 - w3 + pc.a * (v2 * v2 - v3 * v3)) / d;

    pc.c = -(w3 + pc.a * v3 * v3 + pc.b * v3);

    return pc;
  }

  /**
   * Calculates the three polynomial polar coefficients from
   * two pairs of horizontal vs. vertical speed. The first pair defines
   * the point where the polar is flat (derivative equals zero)!
   */
  static constexpr PolarCoefficients From2VW(double v1, double v2,
                                             double w1, double w2) noexcept {
    PolarCoefficients pc;

    auto d = (v2 - v1) * (v2 - v1);
    pc.a = d == 0
      ? 0.
      : (w2 - w1) / d;
    pc.b = -2 * pc.a * v1;
    pc.c = pc.a * v1 * v1 + w1;

    return pc;
  }
};
