// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "ThermalDisplay.hpp"
#include "Look/MapLook.hpp"
#include "ui/canvas/Icon.hpp"
#ifdef HAVE_SKYLINES_TRACKING
#include "Tracking/SkyLines/Data.hpp"
#endif
#ifdef HAVE_HTTP
#include "net/client/tim/Glue.hpp"
#include "net/client/tim/Thermal.hpp"
#endif

template<typename Sources, typename GetLocation>
static void
DrawDriftedThermalIcons(Canvas &canvas, const MaskedIcon &icon,
                        const WindowProjection &projection,
                        const Sources &sources,
                        const double aircraft_altitude,
                        GetLocation get_location) noexcept
{
  if (!ThermalDisplay::IsVisible(projection.GetMapScale()))
    return;

  for (const auto &source : sources) {
    const GeoPoint location = get_location(source, aircraft_altitude);
    if (!location.IsValid())
      continue;

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

  if (!ThermalDisplay::IsVisible(render_projection.GetMapScale()))
    return;

  // draw only at close map scales in non-circling mode

  const SpeedVector wind = calculated.wind_available
    ? calculated.wind
    : SpeedVector::Zero();
  DrawDriftedThermalIcons(
    canvas, look.thermal_source_icon, render_projection,
    thermal_locator.sources, basic.nav_altitude,
    [&wind](const ThermalSource &source, double aircraft_altitude) {
      return ThermalDisplay::GetLocation(source, aircraft_altitude, wind);
    });

#ifdef HAVE_SKYLINES_TRACKING
  const auto &cloud_settings = GetComputerSettings().tracking.cloud;
  if (cloud_settings.show_thermals && skylines_data != nullptr) {
    const std::lock_guard lock{skylines_data->mutex};
    for (auto &i : skylines_data->thermals) {
      // TODO: apply wind drift
      if (auto p = render_projection.GeoToScreenIfVisible(i.bottom_location))
        look.thermal_source_icon.Draw(canvas, *p);
    }
  }
#endif

#ifdef HAVE_HTTP
  if (tim_glue != nullptr && GetComputerSettings().weather.enable_tim) {
    const auto lock = tim_glue->Lock();
    for (const auto &i : tim_glue->Get())
      if (auto p = render_projection.GeoToScreenIfVisible(i.location))
        look.thermal_source_icon.Draw(canvas, *p);
  }
#endif
}
