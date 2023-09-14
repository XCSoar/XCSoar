// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WarningComputer.hpp"
#include "Settings.hpp"
#include "NMEA/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"

using namespace std::chrono;

WarningComputer::WarningComputer(const AirspaceWarningConfig &_config,
                                 Airspaces &_airspaces)
  :airspaces(_airspaces),
   manager(_config, airspaces),
   protected_manager(manager)
{
}

void
WarningComputer::Update(const ComputerSettings &settings_computer,
                        const MoreData &basic,
                        const DerivedInfo &calculated,
                        AirspaceWarningsInfo &result)
{
  if (!basic.time_available)
    return;

  const auto dt = delta_time.Update(basic.time, seconds{1}, seconds{20});
  if (dt.count() < 0)
    /* time warp */
    Reset();

  if (dt.count() <= 0)
    return;

  airspaces.SetFlightLevels(settings_computer.pressure);

  AirspaceActivity day(calculated.date_time_local.day_of_week);
  airspaces.SetActivity(day);

  if (!settings_computer.airspace.enable_warnings ||
      !basic.location_available || !basic.NavAltitudeAvailable()) {
    if (initialised) {
      initialised = false;
      protected_manager.Clear();
    }

    return;
  }

  const AircraftState as = ToAircraftState(basic, calculated);
  ProtectedAirspaceWarningManager::ExclusiveLease lease(protected_manager);

  lease->SetConfig(settings_computer.airspace.warnings);

  if (!initialised) {
    initialised = true;
    lease->Reset(as);
  }

  if (lease->Update(as, settings_computer.polar.glide_polar_task,
                    calculated.task_stats,
                    calculated.circling,
                    round<duration<unsigned>>(dt)))
    result.latest.Update(basic.clock);
}
