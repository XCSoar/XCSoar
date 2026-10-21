// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapWindow.hpp"
#include "Look/MapLook.hpp"
#include "Observation/Database.hpp"
#include "Observation/Publish.hpp"
#include "Engine/Observation/Store.hpp"
#include "ui/canvas/Icon.hpp"
#include "Tracking/Features.hpp"
#include "net/http/Features.hpp"

void
MapWindow::DrawThermalEstimate(Canvas &canvas) const noexcept
{
  if (observations == nullptr || render_projection.GetMapScale() > 4000)
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
