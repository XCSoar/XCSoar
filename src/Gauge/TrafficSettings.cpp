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
  gauge_location = GaugeLocation::AUTO;
}
