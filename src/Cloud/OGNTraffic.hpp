// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/Boost/GeoPoint.hpp"

#include <boost/intrusive/list.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

/**
 * Stable 31-bit FNV-1a folded into the OGN pilot namespace.
 */
[[gnu::pure]]
uint32_t
OGNPilotIdFromStation(std::string_view station_id) noexcept;

/**
 * Encode a FLARM address into the OGN #TrafficResponsePacket::Traffic::pilot_id
 * namespace (distinct from sequential XCSoar-cloud client ids).
 */
[[gnu::const]]
uint32_t
OGNPilotIdFromFlarm(uint32_t flarm_id) noexcept;

struct OGNTrafficEntry
  : std::enable_shared_from_this<OGNTrafficEntry>,
    boost::intrusive::list_base_hook<
      boost::intrusive::link_mode<boost::intrusive::normal_link>> {
  const std::string station_id;

  GeoPoint location;
  int altitude = 0;
  bool altitude_valid = false;

  std::chrono::steady_clock::time_point stamp;

  unsigned track_deg = 0;
  bool track_valid = false;

  uint32_t flarm_id = 0;
  bool flarm_valid = false;

  unsigned aircraft_type = 0;

  /** Cached #OGNPilotIdFromStation(station_id); set on insert. */
  uint32_t pilot_id = 0;

  /** Registration/callsign when known (from OGN APRS or derived). */
  std::string callsign;

  template<typename S>
  OGNTrafficEntry(S &&_station, const GeoPoint &_loc, int _altitude,
                  bool _altitude_valid,
                  unsigned _track_deg, bool _track_valid,
                  uint32_t _flarm_id, bool _flarm_valid,
                  unsigned _aircraft_type)
    :station_id(std::forward<S>(_station)), location(_loc),
     altitude(_altitude), altitude_valid(_altitude_valid),
     stamp(std::chrono::steady_clock::now()),
     track_deg(_track_deg), track_valid(_track_valid),
     flarm_id(_flarm_id), flarm_valid(_flarm_valid),
     aircraft_type(_aircraft_type) {}
};

[[gnu::pure]]
uint32_t
OGNTrafficPilotId(const OGNTrafficEntry &t) noexcept;

using OGNTrafficPtr = std::shared_ptr<OGNTrafficEntry>;

struct OGNTrafficIndexable {
  typedef GeoPoint result_type;

  [[gnu::pure]]
  result_type operator()(const OGNTrafficPtr &t) const noexcept {
    return t->location;
  }
};

namespace OGNTrafficDetail {

struct StationHash {
  using is_transparent = void;

  [[gnu::pure]]
  std::size_t operator()(std::string_view s) const noexcept {
    return std::hash<std::string_view>{}(s);
  }

  std::size_t operator()(const std::string &s) const noexcept {
    return (*this)(std::string_view(s));
  }
};

struct StationEqual {
  using is_transparent = void;

  bool operator()(std::string_view a, std::string_view b) const noexcept {
    return a == b;
  }

  bool operator()(std::string_view a, const std::string &b) const noexcept {
    return a == std::string_view(b);
  }

  bool operator()(const std::string &a, std::string_view b) const noexcept {
    return std::string_view(a) == b;
  }

  bool operator()(const std::string &a, const std::string &b) const noexcept {
    return a == b;
  }
};

} // namespace OGNTrafficDetail

/**
 * In-memory OGN-derived traffic positions (not persisted in cloud DB).
 *
 * CRUD-style API:
 * - Create / update: #Upsert
 * - Read: #Find, #QueryWithinRange
 * - Delete: #Erase, #Expire (time threshold), #clear
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

  std::unordered_map<std::string, OGNTrafficPtr,
                     OGNTrafficDetail::StationHash,
                     OGNTrafficDetail::StationEqual>
    by_station;

  std::unordered_map<uint32_t, OGNTrafficPtr> by_pilot_id;

public:
  OGNTrafficContainer();
  ~OGNTrafficContainer();

  OGNTrafficContainer(const OGNTrafficContainer &) = delete;
  OGNTrafficContainer &operator=(const OGNTrafficContainer &) = delete;

  void clear() noexcept;

  bool empty() const noexcept {
    return list.empty();
  }

  [[gnu::pure]]
  OGNTrafficEntry *FindByPilotId(uint32_t pilot_id) const noexcept;

  OGNTrafficEntry &Upsert(std::string_view station_id, const GeoPoint &location,
                          int altitude, bool altitude_valid,
                          unsigned track_deg, bool track_valid,
                          uint32_t flarm_id, bool flarm_valid,
                          unsigned aircraft_type,
                          std::string_view callsign);

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
