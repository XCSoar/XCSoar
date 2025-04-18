// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "net/client/tim/Thermal.hpp"

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
MapItemListBuilder::AddThermals(const ThermalLocatorInfo &thermals,
                                const MoreData &basic,
                                const DerivedInfo &calculated)
{
  for (const auto &t : thermals.sources) {
    if (list.full())
      break;

    // find height difference
    if (basic.nav_altitude < t.ground_height)
      continue;

    GeoPoint loc = calculated.wind_available
      ? t.CalculateAdjustedLocation(basic.nav_altitude, calculated.wind)
      : t.location;

    if (location.DistanceS(loc) < range)
      list.append(new ThermalMapItem(t));
  }
}

void
MapItemListBuilder::AddThermals(std::span<const TIM::Thermal> thermals) noexcept
{
  for (const auto &i : thermals) {
    if (list.full())
      break;

    if (location.DistanceS(i.location) > range)
      continue;

    ThermalSource source;
    source.location = i.location;
    source.ground_height = 0; // TODO
    source.lift_rate = i.climb_rate;
    // TODO source.time = i.time;

    list.append(new ThermalMapItem(source));
  }
}
