// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/MoreData.hpp"

void
MoreData::Reset() noexcept
{
  nav_altitude = 0;
  energy_height = 0;
  TE_altitude = 0;

  gps_vario = 0;
  gps_vario_available.Clear();

  netto_vario = 0;

  brutto_vario = 0;
  brutto_vario_available.Clear();

  NMEAInfo::Reset();
}
