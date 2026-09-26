// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "ThermalDisplay.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Look/MapLook.hpp"
#include "LogFile.hpp"
#include "ui/canvas/Icon.hpp"
#ifdef HAVE_SKYLINES_TRACKING
#include "Tracking/SkyLines/Data.hpp"
#endif
#ifdef HAVE_HTTP
#include "net/client/tim/Glue.hpp"
#include "net/client/tim/Thermal.hpp"
#endif

template<typename Sources, typename GetLocation, typename GetIcon>
static void
DrawDriftedThermalIcons(Canvas &canvas, const WindowProjection &projection,
                        const Sources &sources,
                        const double aircraft_altitude,
                        GetLocation get_location,
                        GetIcon get_icon) noexcept
{
  if (!ThermalDisplay::IsVisible(projection.GetMapScale()))
    return;

  for (const auto &source : sources) {
    const GeoPoint location = get_location(source, aircraft_altitude);
    if (!location.IsValid())
      continue;

    if (auto p = projection.GeoToScreenIfVisible(location))
      get_icon(source).Draw(canvas, *p);
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
    canvas, render_projection, thermal_locator.sources, basic.nav_altitude,
    [&wind](const ThermalSource &source, double aircraft_altitude) {
      return ThermalDisplay::GetLocation(source, aircraft_altitude, wind);
    },
    [this](const ThermalSource &) -> const MaskedIcon & {
      return look.thermal_source_icon;
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

void
MapWindow::DrawFlarmThermals(Canvas &canvas) const noexcept
{
  if (!ThermalDisplay::IsTrafficVisible(
        GetMapSettings().show_flarm_on_map,
        render_projection.GetMapScale()))
    return;

  const MoreData &basic = Basic();
  const DerivedInfo &calculated = Calculated();
  const auto selected_wind = calculated.wind_available
    ? calculated.wind
    : SpeedVector::Zero();
  const double mac_cready =
    GetComputerSettings().polar.glide_polar_task.GetMC();

  DrawDriftedThermalIcons(
    canvas, render_projection, calculated.traffic_thermals.sources,
    basic.nav_altitude,
    [&selected_wind](const TrafficThermalSource &source,
                     double aircraft_altitude) {
      const GeoPoint location =
        ThermalDisplay::GetLocation(source, aircraft_altitude);
      if (location.IsValid())
        LogDebug("FLARM thermal cluster={} display={:.6f},{:.6f} "
                 "ownship_altitude={:.1f} reporting_lift={:.2f} "
                 "geometry_lift={:.2f} stored_wind={:.1f}@{:.1f} "
                 "selected_wind={:.1f}@{:.1f}",
                 source.cluster_serial,
                 location.latitude.Degrees(), location.longitude.Degrees(),
                 aircraft_altitude, source.thermal.lift_rate,
                 source.geometry_lift_rate,
                 source.geometry_wind.norm,
                 source.geometry_wind.bearing.Degrees(),
                 selected_wind.norm, selected_wind.bearing.Degrees());
      return location;
    },
    [this, mac_cready](const TrafficThermalSource &source)
      -> const MaskedIcon & {
      return ThermalDisplay::GetFlarmThermalIcon(
        look, source.thermal.lift_rate, mac_cready);
    });
}
