// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"

void
WindSettings::SetDefaults() noexcept
{
  circling_wind = true;
  zig_zag_wind = true;
  external_wind = true;
  manual_wind_available.Clear();
}
