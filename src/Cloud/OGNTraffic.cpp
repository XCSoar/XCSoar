// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OGNTraffic.hpp"

#include "OGNAprs.hpp"
#include "Tracking/SkyLines/TrafficExtensions.hpp"
#include "Geo/Boost/RangeBox.hpp"

using SkyLinesTracking::OGN_PILOT_ID_MASK;

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/strategies/strategies.hpp>

uint32_t
OGNPilotIdFromStation(std::string_view station_id) noexcept
{
  uint32_t h = 2166136261u;
  for (unsigned char ch : station_id)
    h = (h ^ uint32_t(ch)) * 16777619u;
  return OGN_PILOT_ID_MASK | (h & 0x7fffffffu);
}

uint32_t
OGNPilotIdFromFlarm(uint32_t flarm_id) noexcept
{
  return OGN_PILOT_ID_MASK | (flarm_id & 0xffffffu);
}

uint32_t
OGNTrafficPilotId(const OGNTrafficEntry &t) noexcept
{
  if (t.flarm_valid)
    return OGNPilotIdFromFlarm(t.flarm_id);

  if (t.pilot_id != 0)
    return t.pilot_id;

  return OGNPilotIdFromStation(t.station_id);
}

bool
IsForwardableOgnTraffic(const OGNTrafficEntry &t) noexcept
{
  if (!t.flarm_valid)
    return false;

  if (t.address_type == OGN_ADDRESS_TYPE_TRACKER)
    return false;

  return true;
}

OGNTrafficContainer::OGNTrafficContainer() = default;

OGNTrafficContainer::~OGNTrafficContainer()
{
  clear();
}

void
OGNTrafficContainer::clear() noexcept
{
  while (!list.empty())
    Remove(list.back());
  by_pilot_id.clear();
}

void
OGNTrafficContainer::Insert(OGNTrafficEntry &t) noexcept
{
  list.push_front(t);
  rtree.insert(t.shared_from_this());
}

void
OGNTrafficContainer::Remove(OGNTrafficEntry &t) noexcept
{
  by_station.erase(t.station_id);
  by_pilot_id.erase(t.pilot_id);
  list.erase(list.iterator_to(t));
  rtree.remove(t.shared_from_this());
}

OGNTrafficEntry *
OGNTrafficContainer::FindByPilotId(uint32_t pilot_id) const noexcept
{
  const auto it = by_pilot_id.find(pilot_id);
  if (it == by_pilot_id.end())
    return nullptr;

  return it->second.get();
}

OGNTrafficEntry &
OGNTrafficContainer::Upsert(std::string_view station_sv,
                            const GeoPoint &location, int altitude,
                            bool altitude_valid,
                            unsigned track_deg, bool track_valid,
                            uint32_t flarm_id, bool flarm_valid,
                            unsigned aircraft_type, unsigned address_type,
                            std::string_view callsign)
{
  auto it = by_station.find(station_sv);
  const auto now = std::chrono::steady_clock::now();

  if (it == by_station.end()) {
    std::string station{station_sv};
    auto p = std::make_shared<OGNTrafficEntry>(
      std::move(station), location, altitude, altitude_valid,
      track_deg, track_valid,
      flarm_id, flarm_valid, aircraft_type, address_type);
    if (!callsign.empty())
      p->callsign.assign(callsign.data(), callsign.size());
    p->pilot_id = OGNTrafficPilotId(*p);
    by_station.emplace(p->station_id, p);
    by_pilot_id.emplace(p->pilot_id, p);
    Insert(*p);
    return *p;
  }

  OGNTrafficEntry &e = *it->second;
  const uint32_t old_pilot_id = e.pilot_id;

  if (e.location != location) {
    const OGNTrafficPtr ptr = e.shared_from_this();
    rtree.remove(ptr);
    e.location = location;
    rtree.insert(ptr);
  }

  e.altitude = altitude;
  e.altitude_valid = altitude_valid;
  e.stamp = now;
  e.track_deg = track_deg;
  e.track_valid = track_valid;
  e.flarm_id = flarm_id;
  e.flarm_valid = flarm_valid;
  e.aircraft_type = aircraft_type;
  e.address_type = address_type;
  if (!callsign.empty())
    e.callsign.assign(callsign.data(), callsign.size());
  e.pilot_id = OGNTrafficPilotId(e);

  if (e.pilot_id != old_pilot_id) {
    by_pilot_id.erase(old_pilot_id);
    by_pilot_id.emplace(e.pilot_id, it->second);
  }

  list.erase(list.iterator_to(e));
  list.push_front(e);
  return e;
}

void
OGNTrafficContainer::Expire(std::chrono::steady_clock::time_point before) noexcept
{
  while (!list.empty() && list.back().stamp < before)
    Remove(list.back());
}

OGNTrafficContainer::query_iterator_range
OGNTrafficContainer::QueryWithinRange(GeoPoint location,
                                      double range) const noexcept
{
  const auto q =
    boost::geometry::index::intersects(BoostRangeBox(location, range));
  return {rtree.qbegin(q), rtree.qend()};
}
