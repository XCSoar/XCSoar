// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NavigatorSettings.hpp"

void
NavigatorSettings::SetDefaults() noexcept
{
  navigator_lite_1_line_height = 6;
  navigator_lite_2_lines_height = 8;
  navigator_height = 9;
  navigator_detailed_height = 11;

  navigator_swipe = false;
  navigator_altitude_type = NavigatorWidgetAltitudeType::WP_AltArrival;
}
