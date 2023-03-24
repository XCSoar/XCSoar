// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindowBlackboard.hpp"

void
MapWindowBlackboard::ReadComputerSettings(const ComputerSettings
					      &settings)
{
  computer_settings = settings;
}

void
MapWindowBlackboard::ReadMapSettings(const MapSettings
				     &settings)
{
  settings_map = settings;
}

void
MapWindowBlackboard::ReadBlackboard(const MoreData &nmea_info,
				    const DerivedInfo &derived_info)
{
  gps_info = nmea_info;
  calculated_info = derived_info;
}

