/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "GlideRatioComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

void
GlideRatioComputer::Reset()
{
  gr_calculator_initialised = false;
}

void
GlideRatioComputer::Compute(const MoreData &basic, const MoreData &last_basic,
                            const DerivedInfo &calculated,
                            VarioInfo &vario_info,
                            const ComputerSettings &settings)
{
  if (!last_basic.location_available ||
      !basic.NavAltitudeAvailable() || !last_basic.NavAltitudeAvailable()) {
    vario_info.gr = INVALID_GR;
    return;
  }

  if (!basic.location_available.Modified(last_basic.location_available))
    return;

  fixed DistanceFlown = basic.location.Distance(last_basic.location);

  // Glide ratio over ground
  vario_info.gr =
    UpdateGR(vario_info.gr, DistanceFlown,
             last_basic.nav_altitude - basic.nav_altitude, fixed(0.1));

  if (calculated.flight.flying && !calculated.circling) {
    if (!gr_calculator_initialised) {
      gr_calculator_initialised = true;
      gr_calculator.Initialize(settings);
    }

    gr_calculator.Add((int)DistanceFlown, (int)basic.nav_altitude);
    vario_info.average_gr = gr_calculator.Calculate();
  } else
    gr_calculator_initialised = false;
}
