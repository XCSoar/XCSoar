// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "ThermalDisplay.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Look/MapLook.hpp"
#include "Observation/Database.hpp"
#include "Observation/Publish.hpp"
#include "Engine/Observation/Store.hpp"
#include "LogFile.hpp"
#include "ui/canvas/Icon.hpp"
#include "Tracking/Features.hpp"
#include "net/http/Features.hpp"

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
  if (observations == nullptr ||
      !ThermalDisplay::IsVisible(render_projection.GetMapScale()))
    return;

  const MoreData &basic = Basic();
  const DerivedInfo &calculated = Calculated();
  const ComputerSettings &settings = GetComputerSettings();

  bool show_cloud = false;
#ifdef HAVE_TRACKING
  show_cloud = settings.tracking.cloud.show_thermals;
#endif

  bool show_tim = false;
#ifdef HAVE_HTTP
  show_tim = settings.weather.enable_tim;
#endif

  const SpeedVector wind = calculated.wind_available
    ? calculated.wind
    : SpeedVector::Zero();

  ThermalRows raw, visible, shown;
  observations->CopyThermals(raw);
  CollectVisibleThermals(raw, visible, basic.nav_altitude, wind,
                         show_cloud, show_tim);
  SelectThermals(visible, shown, THERMAL_DUPLICATE_RANGE_M);

  for (const auto &row : shown)
    if (auto p = render_projection.GeoToScreenIfVisible(row.location))
      look.thermal_source_icon.Draw(canvas, *p);
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
