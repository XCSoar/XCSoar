/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Settings.hpp"

void
ThermalBandComputer::Reset()
{
  last_vario_available.Clear();
  in_encounter = false;
}

void
ThermalBandComputer::Compute(const MoreData &basic,
                             const DerivedInfo &calculated,
                             ThermalEncounterBand &teb,
                             ThermalEncounterCollection &tec,
                             const ComputerSettings &settings)
{
  if (!basic.NavAltitudeAvailable())
    return;

  const auto h_thermal = basic.nav_altitude;

  last_vario_available.FixTimeWarp(basic.brutto_vario_available);

  if (basic.brutto_vario_available.Modified(last_vario_available)) {
    last_vario_available = basic.brutto_vario_available;

    // only do this if in circling mode
    if (calculated.circling) {
      teb.AddSample(basic.time, h_thermal);
      in_encounter = true;
    } else if (in_encounter) {
      tec.Merge(teb);
      teb.Reset();
      in_encounter = false;
    }
  }
}
