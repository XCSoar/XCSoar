// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioInfo.hpp"
#include "Computer/GlideRatioCalculator.hpp"

void
VarioInfo::Clear()
{
  sink_rate = 0;
  average = netto_average = 0;
  cruise_gr = INVALID_GR;
  average_gr = 0;
  gr = INVALID_GR;
  ld_vario = INVALID_GR;

  lift_database.Clear();
}
