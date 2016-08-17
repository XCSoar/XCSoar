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

#include "MapWindow.hpp"
#include "Look/MapLook.hpp"
#include "Screen/Icon.hpp"
#include "Tracking/SkyLines/Data.hpp"

template<typename T>
static void
DrawThermalSources(Canvas &canvas, const MaskedIcon &icon,
                   const WindowProjection &projection,
                   const T &sources,
                   const double aircraft_altitude,
                   const SpeedVector &wind)
{
  for (const auto &source : sources) {
    // find height difference
    if (aircraft_altitude < source.ground_height)
      continue;

    // draw thermal at location it would be at the glider's height
    GeoPoint location = wind.IsNonZero()
      ? source.CalculateAdjustedLocation(aircraft_altitude, wind)
      : source.location;

    // draw if it is in the field of view
    PixelPoint pt;
    if (projection.GeoToScreenIfVisible(location, pt))
      icon.Draw(canvas, pt);
  }
}

void
MapWindow::DrawThermalEstimate(Canvas &canvas) const
{
  const MoreData &basic = Basic();
  const DerivedInfo &calculated = Calculated();
  const ThermalLocatorInfo &thermal_locator = calculated.thermal_locator;

  if (render_projection.GetMapScale() > 4000)
    return;

  // draw only at close map scales in non-circling mode

  DrawThermalSources(canvas, look.thermal_source_icon, render_projection,
                     thermal_locator.sources, basic.nav_altitude,
                     calculated.wind_available
                     ? calculated.wind : SpeedVector::Zero());

  const auto &cloud_settings = ComputerSettings().tracking.skylines.cloud;
  if (cloud_settings.show_thermals && skylines_data != nullptr) {
    ScopeLock protect(skylines_data->mutex);
    for (auto &i : skylines_data->thermals) {
      // TODO: apply wind drift
      PixelPoint pt;
      if (render_projection.GeoToScreenIfVisible(i.bottom_location, pt))
        look.thermal_source_icon.Draw(canvas, pt);
    }
  }
}
