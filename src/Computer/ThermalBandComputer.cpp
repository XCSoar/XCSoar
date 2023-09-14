// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
                             ThermalEncounterCollection &tec)
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
