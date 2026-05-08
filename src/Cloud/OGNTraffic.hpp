// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/Boost/GeoPoint.hpp"

#include <boost/intrusive/list.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

/**
 * Synthetic pilot ids from OGN never overlap XCSoar-cloud sequential client
 * ids (which stay in the low range).
 */
[[gnu::const]]
constexpr uint32_t
OGNPilotIdMask() noexcept
{
  return 0x80000000u;
}

/**
 * Stable 31-bit FNV-1a folded into the OGN pilot namespace.
 */
[[gnu::pure]]
uint32_t
OGNPilotIdFromStation(std::string_view station_id) noexcept;

struct OGNTrafficEntry
  : std::enable_shared_from_this<OGNTrafficEntry>,
    boost::intrusive::list_base_hook<
      boost::intrusive::link_mode<boost::intrusive::normal_link>> {
  const std::string station_id;

  GeoPoint location;
  int altitude = 0;

  std::chrono::steady_clock::time_point stamp;

  unsigned track_deg = 0;
  bool track_valid = false;

  uint32_t flarm_id = 0;
  bool flarm_valid = false;

  unsigned aircraft_type = 0;

  template<typename S>
  OGNTrafficEntry(S &&_station, const GeoPoint &_loc, int _altitude,
                  unsigned _track_deg, bool _track_valid,
                  uint32_t _flarm_id, bool _flarm_valid,
                  unsigned _aircraft_type)
    :station_id(std::forward<S>(_station)), location(_loc),
     altitude(_altitude),
     stamp(std::chrono::steady_clock::now()),
     track_deg(_track_deg), track_valid(_track_valid),
     flarm_id(_flarm_id), flarm_valid(_flarm_valid),
     aircraft_type(_aircraft_type) {}
};

using OGNTrafficPtr = std::shared_ptr<OGNTrafficEntry>;

struct OGNTrafficIndexable {
  typedef GeoPoint result_type;

  [[gnu::pure]]
  result_type operator()(const OGNTrafficPtr &t) const noexcept {
    return t->location;
  }
};

/**
 * In-memory OGN-derived traffic positions (not persisted in cloud DB).
 */
class OGNTrafficContainer {
  typedef boost::geometry::index::rtree<
    OGNTrafficPtr, boost::geometry::index::rstar<16>, OGNTrafficIndexable>
    Tree;

  typedef boost::intrusive::list<
    OGNTrafficEntry,
    boost::intrusive::constant_time_size<false>>
    List;

  Tree rtree;
  List list;

  std::unordered_map<std::string, OGNTrafficPtr> by_station;

public:
  OGNTrafficContainer();
  ~OGNTrafficContainer();

  OGNTrafficContainer(const OGNTrafficContainer &) = delete;
  OGNTrafficContainer &operator=(const OGNTrafficContainer &) = delete;

  void clear() noexcept;

  bool empty() const noexcept {
    return list.empty();
  }

  OGNTrafficEntry &Upsert(std::string_view station_id, const GeoPoint &location,
                          int altitude, unsigned track_deg, bool track_valid,
                          uint32_t flarm_id, bool flarm_valid,
                          unsigned aircraft_type) noexcept;

  void Expire(std::chrono::steady_clock::time_point before) noexcept;

  typedef Tree::const_query_iterator query_iterator;
  typedef boost::iterator_range<query_iterator> query_iterator_range;

  [[gnu::pure]]
  query_iterator_range QueryWithinRange(GeoPoint location,
                                        double range) const noexcept;

private:
  void Insert(OGNTrafficEntry &t) noexcept;
  void Remove(OGNTrafficEntry &t) noexcept;
};
