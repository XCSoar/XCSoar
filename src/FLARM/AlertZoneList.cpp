// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AlertZoneList.hpp"
#include "Engine/Airspace/Airspaces.hpp"

void
AlertZoneList::UpdateAirspaces(Airspaces &airspaces) const noexcept
{
  airspaces.Clear();

  for (const auto &zone : list) {
    if (auto airspace = zone.ToAirspace())
      airspaces.Add(std::move(airspace));
  }

  airspaces.Optimise();
}

