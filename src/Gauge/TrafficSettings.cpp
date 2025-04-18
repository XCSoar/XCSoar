// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficSettings.hpp"

void
TrafficSettings::SetDefaults() noexcept
{
  enable_gauge = true;
  auto_close_dialog = false;
  auto_zoom = true;
  north_up = false;
  radar_zoom = 4;
  gauge_location = GaugeLocation::BOTTOM_RIGHT_AVOID_IB;
}
