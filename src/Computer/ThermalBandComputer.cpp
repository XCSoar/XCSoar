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

#include "ThermalBandComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "ComputerSettings.hpp"

void
ThermalBandComputer::Reset()
{
  last_vario_available.Clear();
}

void
ThermalBandComputer::Compute(const MoreData &basic,
                             const DerivedInfo &calculated,
                             ThermalBandInfo &tbi,
                             const ComputerSettings &settings)
{
  if (!basic.NavAltitudeAvailable())
    return;

  const fixed h_safety =
    settings.task.route_planner.safety_height_terrain +
    calculated.GetTerrainBaseFallback();

  tbi.working_band_height = basic.TE_altitude - h_safety;
  if (negative(tbi.working_band_height)) {
    tbi.working_band_fraction = fixed(0);
    return;
  }

  const fixed max_height = tbi.max_thermal_height;
  if (positive(max_height))
    tbi.working_band_fraction = tbi.working_band_height / max_height;
  else
    tbi.working_band_fraction = fixed(1);

  tbi.working_band_ceiling = std::max(max_height + h_safety,
                                      basic.TE_altitude);


  last_vario_available.FixTimeWarp(basic.brutto_vario_available);
  if (basic.brutto_vario_available.Modified(last_vario_available)) {
    last_vario_available = basic.brutto_vario_available;

    // JMW TODO accuracy: Should really work out dt here,
    //           but i'm assuming constant time steps

    if (tbi.max_thermal_height == fixed(0))
      tbi.max_thermal_height = tbi.working_band_height;

    // only do this if in thermal and have been climbing
    if (calculated.circling && calculated.turning &&
        positive(calculated.average))
      tbi.Add(tbi.working_band_height, basic.brutto_vario);
  }
}
