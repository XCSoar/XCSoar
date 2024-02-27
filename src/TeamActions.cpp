// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TeamActions.hpp"
#include "Interface.hpp"
#include "FLARM/Details.hpp"
#include "FLARM/TrafficDatabases.hpp"
#include "FLARM/Global.hpp"

void
TeamActions::TrackFlarm(FlarmId id, const TCHAR *callsign) noexcept
{
  TeamCodeSettings &settings =
    CommonInterface::SetComputerSettings().team_code;

  if (callsign == nullptr)
    callsign = FlarmDetails::LookupCallsign(id);

  settings.TrackFlarm(id, callsign);

  if (traffic_databases != nullptr)
    traffic_databases->team_flarm_id = id;
}
