/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "WarningComputer.hpp"
#include "Settings.hpp"
#include "NMEA/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"

WarningComputer::WarningComputer(Airspaces &_airspaces)
  :airspaces(_airspaces),
   manager(airspaces),
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

  const fixed dt = delta_time.Update(basic.time, fixed(1), fixed(20));
  if (negative(dt))
    /* time warp */
    Reset();

  if (!positive(dt))
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
                    uround(dt)))
    result.latest.Update(basic.clock);
}
