// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalBandLook.hpp"
#include "Screen/Layout.hpp"
#include "Look/Colors.hpp"

void
ThermalBandLook::Initialise(bool _inverse, Color sky_color)
{
  const uint8_t alpha = ALPHA_OVERLAY;

  inverse = _inverse;

  brush_active.Create(ColorWithAlpha(sky_color, alpha));
  brush_inactive.Create(ColorWithAlpha(DarkColor(sky_color), alpha/2));

  pen_active.Create(Layout::ScalePenWidth(1), DarkColor(sky_color));
  pen_inactive.Create(Layout::ScalePenWidth(1), sky_color);

  white_pen.Create(Layout::ScalePenWidth(2), COLOR_WHITE);
  black_pen.Create(Layout::ScalePenWidth(2), COLOR_BLACK);

  working_band_pen.Create(Layout::ScalePenWidth(2), COLOR_GRAY);
}
