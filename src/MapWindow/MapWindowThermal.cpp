// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "Look/MapLook.hpp"
#include "ui/canvas/Icon.hpp"
#include "Tracking/SkyLines/Data.hpp"
#include "net/client/tim/Glue.hpp"
#include "net/client/tim/Thermal.hpp"

template<typename T>
static void
DrawThermalSources(Canvas &canvas, const MaskedIcon &icon,
                   const WindowProjection &projection,
                   const T &sources,
                   const double aircraft_altitude,
                   const SpeedVector &wind) noexcept
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
    if (auto p = projection.GeoToScreenIfVisible(location))
      icon.Draw(canvas, *p);
  }
}

void
MapWindow::DrawThermalEstimate(Canvas &canvas) const noexcept
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

  const auto &cloud_settings = GetComputerSettings().tracking.skylines.cloud;
  if (cloud_settings.show_thermals && skylines_data != nullptr) {
    const std::lock_guard lock{skylines_data->mutex};
    for (auto &i : skylines_data->thermals) {
      // TODO: apply wind drift
      if (auto p = render_projection.GeoToScreenIfVisible(i.bottom_location))
        look.thermal_source_icon.Draw(canvas, *p);
    }
  }

  if (tim_glue != nullptr && GetComputerSettings().weather.enable_tim) {
    const auto lock = tim_glue->Lock();
    for (const auto &i : tim_glue->Get())
      if (auto p = render_projection.GeoToScreenIfVisible(i.location))
        look.thermal_source_icon.Draw(canvas, *p);
  }
}
