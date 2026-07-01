// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TileValue.hpp"

#include <cmath>

namespace EDL {

bool
DecodeAscendancyPixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                      double &value_mps) noexcept
{
  if (r == 0 && g == 0 && b == 0) {
    if (a == 0)
      return false;

    value_mps = double(a) / 25.5 - 5.0;
    return true;
  }

  /* Full-saturation palette colours encode magnitude in alpha. */
  if (r == 255 && g == 255 && b == 0 && a > 0) {
    value_mps = 5.0 - double(a) / 25.5;
    return true;
  }

  if (r == 0 && g == 255 && b == 255 && a > 0) {
    value_mps = double(a) / 25.5 - 5.0;
    return true;
  }

  /* Pale yellow/cyan ramps and other hues use RGB intensity. */
  const int grb = int(g) - int(r) - int(b);
  const int rb = int(r) - int(b);
  if (std::abs(grb) >= std::abs(rb))
    value_mps = double(grb) / 51.0;
  else
    value_mps = double(rb) / 51.0;
  return true;
}

} // namespace EDL
