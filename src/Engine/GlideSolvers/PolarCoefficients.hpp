/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_POLAR_COEFF_HPP
#define XCSOAR_POLAR_COEFF_HPP

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

#endif
