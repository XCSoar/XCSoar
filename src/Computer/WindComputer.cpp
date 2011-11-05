/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "WindComputer.hpp"
#include "SettingsComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

void
WindComputer::Reset()
{
  last_circling = false;

  circling_wind.reset();
  wind_ekf.reset();
  wind_store.reset();
}

void
WindComputer::Compute(const SETTINGS_COMPUTER &settings,
                      const MoreData &basic, const NMEAInfo &last_basic,
                      DerivedInfo &calculated)
{
  if ((settings.AutoWindMode & D_AUTOWIND_CIRCLING) != 0 &&
      calculated.circling != last_circling)
    circling_wind.slot_newFlightMode(calculated, calculated.TurningLeft(), 0);

last_circling = calculated.circling;

  if (!calculated.flight.flying || !basic.HasTimeAdvancedSince(last_basic))
    return;

  if ((settings.AutoWindMode & D_AUTOWIND_CIRCLING) != 0 &&
      calculated.turn_mode == CLIMB) {
    CirclingWind::Result result = circling_wind.NewSample(basic);
    if (result.IsValid())
      wind_store.SlotMeasurement(basic, result.wind, result.quality);
  }

  if ((settings.AutoWindMode & D_AUTOWIND_ZIGZAG) != 0 &&
      basic.airspeed_available && basic.airspeed_real &&
      basic.true_airspeed > settings.glide_polar_task.GetVTakeoff()) {
    WindEKFGlue::Result result = wind_ekf.Update(basic, calculated);
    if (result.quality > 0) {
      Vector v_wind = Vector(result.wind);
      wind_store.SlotMeasurement(basic, v_wind, result.quality);
    }
  }

  if (settings.AutoWindMode != 0)
    wind_store.SlotAltitude(basic, calculated);
}

void
WindComputer::Select(const SETTINGS_COMPUTER &settings,
                     const NMEAInfo &basic, DerivedInfo &calculated)
{
  if (basic.external_wind_available && settings.ExternalWind) {
    // external wind available
    calculated.wind = basic.external_wind;
    calculated.wind_available = basic.external_wind_available;

  } else if (settings.ManualWindAvailable && settings.AutoWindMode == 0) {
    // manual wind only if available and desired
    calculated.wind = settings.ManualWind;
    calculated.wind_available = settings.ManualWindAvailable;

  } else if (calculated.estimated_wind_available.Modified(settings.ManualWindAvailable)
             && settings.AutoWindMode) {
    // auto wind when available and newer than manual wind
    calculated.wind = calculated.estimated_wind;
    calculated.wind_available = calculated.estimated_wind_available;

  } else if (settings.ManualWindAvailable
             && settings.AutoWindMode) {
    // manual wind overrides auto wind if available
    calculated.wind = settings.ManualWind;
    calculated.wind_available = settings.ManualWindAvailable;

  } else
   // no wind available
   calculated.wind_available.Clear();
}
