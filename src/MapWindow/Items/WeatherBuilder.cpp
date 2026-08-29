// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Builder.hpp"
#include "MapItem.hpp"
#include "List.hpp"
#include "MapWindow/ThermalDisplay.hpp"
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
MapItemListBuilder::AddThermals(const ThermalLocatorInfo &thermals,
                                const MoreData &basic,
                                const DerivedInfo &calculated)
{
  const SpeedVector wind = calculated.wind_available
    ? calculated.wind
    : SpeedVector::Zero();
  AddDriftedThermals(
    list, location, range, thermals.sources,
    [&basic, &wind](const ThermalSource &source) {
      return ThermalDisplay::GetLocation(source, basic.nav_altitude, wind);
    },
    [&basic](const ThermalSource &source) {
      return new ThermalMapItem(source, basic.time);
    });
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

    list.append(new ThermalMapItem(source, TimeStamp::Undefined()));
  }
}
