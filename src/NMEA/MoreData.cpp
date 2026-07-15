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

  filtered_brutto_vario = 0;
  filtered_brutto_vario_available.Clear();

  filtered_netto_vario = 0;
  filtered_netto_vario_available.Clear();

  V_stf = 0;
  V_stf_available.Clear();

  NMEAInfo::Reset();
}
