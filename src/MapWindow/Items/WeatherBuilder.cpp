// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Computer/Settings.hpp"
#include "Observation/Database.hpp"
#include "Observation/Publish.hpp"
#include "Engine/Observation/Store.hpp"
#include "Tracking/Features.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_NOAA
#include "Weather/NOAAStore.hpp"
#endif

#ifdef HAVE_NOAA
void
MapItemListBuilder::AddWeatherStations(NOAAStore &store)
{
  for (auto it = store.begin(), end = store.end(); it != end; ++it) {
    if (list.full())
      break;

    if (it->parsed_metar_available &&
        it->parsed_metar.location_available &&
        location.DistanceS(it->parsed_metar.location) < range)
      list.checked_append(new WeatherStationMapItem(it));
  }
}
#endif

void
MapItemListBuilder::AddThermals(const ObservationDatabase &observations,
                                const MoreData &basic,
                                const DerivedInfo &calculated,
                                const ComputerSettings &settings)
{
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
  observations.CopyThermals(raw);
  CollectVisibleThermals(raw, visible, basic.nav_altitude, wind,
                         show_cloud, show_tim);
  SelectThermals(visible, shown, THERMAL_DUPLICATE_RANGE_M);

  for (const auto &row : shown) {
    if (list.full())
      break;
    if (location.DistanceS(row.location) < range)
      list.append(new ThermalMapItem(row));
  }
}
