// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "MapWindow/ThermalDisplay.hpp"
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

template<typename Sources, typename GetLocation, typename MakeItem>
static void
AddDriftedThermals(MapItemList &list, GeoPoint location, double range,
                   const Sources &sources, GetLocation get_location,
                   MakeItem make_item)
{
  for (const auto &source : sources) {
    if (list.full())
      break;

    const GeoPoint adjusted_location = get_location(source);
    if (!adjusted_location.IsValid())
      continue;

    if (location.DistanceS(adjusted_location) < range)
      list.append(make_item(source));
  }
}

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

void
MapItemListBuilder::AddTrafficThermals(const TrafficThermalInfo &thermals,
                                       const MoreData &basic,
                                       const DerivedInfo &calculated)
{
  (void)calculated;

  AddDriftedThermals(
    list, location, range, thermals.sources,
    [&basic](const TrafficThermalSource &source) {
      return ThermalDisplay::GetLocation(source, basic.nav_altitude);
    },
    [&basic](const TrafficThermalSource &source) {
      return new ThermalMapItem(source, basic.time);
    });
}
