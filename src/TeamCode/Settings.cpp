// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"

void
TeamCodeSettings::SetDefaults()
{
  team_code_reference_waypoint = -1;
  team_flarm_callsign.clear();
  team_flarm_id.Clear();
}

void
TeamCodeSettings::TrackFlarm(FlarmId id, const char *name)
{
  // Start tracking
  team_flarm_id = id;
  team_code.Clear();

  // Set the Teammate callsign
  if (name != nullptr)
    // copy the 3 first chars from the name
    team_flarm_callsign = name;
  else
    team_flarm_callsign.clear();
}
