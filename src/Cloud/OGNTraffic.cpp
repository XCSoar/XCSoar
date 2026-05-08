// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OGNTraffic.hpp"

#include "Geo/Boost/RangeBox.hpp"

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/strategies/strategies.hpp>

uint32_t
OGNPilotIdFromStation(std::string_view station_id) noexcept
{
  uint32_t h = 2166136261u;
  for (unsigned char ch : station_id)
    h = (h ^ uint32_t(ch)) * 16777619u;
  return OGNPilotIdMask() | (h & 0x7fffffffu);
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
  list.erase(list.iterator_to(t));
  rtree.remove(t.shared_from_this());
}

OGNTrafficEntry *
OGNTrafficContainer::Find(std::string_view station_sv) const noexcept
{
  const auto it = by_station.find(station_sv);
  if (it == by_station.end())
    return nullptr;

  return it->second.get();
}

bool
OGNTrafficContainer::Erase(std::string_view station_sv) noexcept
{
  const auto it = by_station.find(station_sv);
  if (it == by_station.end())
    return false;

  Remove(*it->second);
  return true;
}

OGNTrafficEntry &
OGNTrafficContainer::Upsert(std::string_view station_sv,
                            const GeoPoint &location, int altitude,
                            unsigned track_deg, bool track_valid,
                            uint32_t flarm_id, bool flarm_valid,
                            unsigned aircraft_type) noexcept
{
  auto it = by_station.find(station_sv);
  const auto now = std::chrono::steady_clock::now();

  if (it == by_station.end()) {
    std::string station{station_sv};
    auto p = std::make_shared<OGNTrafficEntry>(
      std::move(station), location, altitude, track_deg, track_valid,
      flarm_id, flarm_valid, aircraft_type);
    p->pilot_id = OGNPilotIdFromStation(p->station_id);
    by_station.emplace(p->station_id, p);
    Insert(*p);
    return *p;
  }

  OGNTrafficEntry &e = *it->second;
  if (e.location != location) {
    const OGNTrafficPtr ptr = e.shared_from_this();
    rtree.remove(ptr);
    e.location = location;
    rtree.insert(ptr);
  }

  e.altitude = altitude;
  e.stamp = now;
  e.track_deg = track_deg;
  e.track_valid = track_valid;
  e.flarm_id = flarm_id;
  e.flarm_valid = flarm_valid;
  e.aircraft_type = aircraft_type;

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
