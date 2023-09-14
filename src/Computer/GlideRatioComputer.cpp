// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlideRatioComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

void
GlideRatioComputer::Reset()
{
  gr_calculator_initialised = false;
  last_location_available.Clear();
}

void
GlideRatioComputer::Compute(const MoreData &basic,
                            const DerivedInfo &calculated,
                            VarioInfo &vario_info,
                            const ComputerSettings &settings)
{
  if (!basic.NavAltitudeAvailable()) {
    Reset();
    vario_info.gr = INVALID_GR;
    vario_info.average_gr = 0;
    return;
  }

  if (!last_location_available) {
    /* need two consecutive valid locations; if the previous one
       wasn't valid, skip this iteration and try the next one */
    Reset();
    vario_info.gr = INVALID_GR;
    vario_info.average_gr = 0;

    last_location = basic.location;
    last_location_available = basic.location_available;
    last_altitude = basic.nav_altitude;
    return;
  }

  if (!basic.location_available.Modified(last_location_available))
    return;

  auto DistanceFlown = basic.location.DistanceS(last_location);

  // Glide ratio over ground
  vario_info.gr =
    UpdateGR(vario_info.gr, DistanceFlown,
             last_altitude - basic.nav_altitude, 0.1);

  if (calculated.flight.flying && !calculated.circling) {
    if (!gr_calculator_initialised) {
      gr_calculator_initialised = true;
      gr_calculator.Initialize(settings);
    }

    gr_calculator.Add((int)DistanceFlown, (int)basic.nav_altitude);
    vario_info.average_gr = gr_calculator.Calculate();
  } else
    gr_calculator_initialised = false;

  last_location = basic.location;
  last_location_available = basic.location_available;
  last_altitude = basic.nav_altitude;
}
