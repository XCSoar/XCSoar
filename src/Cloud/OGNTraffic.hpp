// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"
#include "Geo/Boost/GeoPoint.hpp"
#include "util/tstring.hpp"

#include <boost/geometry/index/rtree.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>

/**
 * An OGN aircraft traffic entry.
 */
struct OGNTraffic
    : std::enable_shared_from_this<OGNTraffic>,
      boost::intrusive::list_base_hook<
          boost::intrusive::link_mode<boost::intrusive::normal_link> >,
      boost::intrusive::set_base_hook<
          boost::intrusive::link_mode<boost::intrusive::normal_link> > {
  /**
   * OGN device ID (hex string, e.g., "DD1234")
   */
  tstring device_id;

  /**
   * Callsign/registration (if available from DDB)
   */
  tstring callsign;

  /**
   * Internal numeric ID for traffic responses (unique identifier).
   */
  const unsigned id;

  /**
   * FlarmId/ICAO ID extracted from device ID (0 if not available).
   * This is stored separately from id to allow FLARM database matching.
   */
  unsigned flarm_id;

  /**
   * Time when we most recently received data.
   */
  std::chrono::steady_clock::time_point stamp;

  /**
   * Last known location.
   */
  GeoPoint location;

  /**
   * Last known altitude (meters).
   */
  int altitude;

  /**
   * Track (degrees, 0-359).
   */
  unsigned track;

  /**
   * Ground speed (m/s).
   */
  unsigned speed;

  /**
   * Climb rate (m/s).
   */
  int climb_rate;

  /**
   * Turn rate (degrees per second).
   */
  double turn_rate;

  /**
   * Aircraft type (from FLARM enum).
   */
  FlarmTraffic::AircraftType aircraft_type;

  struct IdCompare {
    [[gnu::pure]]
    bool operator()(const OGNTraffic &a, const OGNTraffic &b) const
    {
      return a.id < b.id;
    }

    [[gnu::pure]]
    bool operator()(unsigned a, const OGNTraffic &b) const
    {
      return a < b.id;
    }

    [[gnu::pure]]
    bool operator()(const OGNTraffic &a, unsigned b) const
    {
      return a.id < b;
    }
  };

  OGNTraffic(unsigned _id, const tstring &_device_id, unsigned _flarm_id = 0)
      : device_id(_device_id), id(_id), flarm_id(_flarm_id),
        stamp(std::chrono::steady_clock::now()), altitude(-1), track(0),
        speed(0), climb_rate(0), turn_rate(0),
        aircraft_type(FlarmTraffic::AircraftType::UNKNOWN)
  {
  }

  void Update(const GeoPoint &_location, int _altitude, unsigned _track,
              unsigned _speed, int _climb_rate, double _turn_rate = 0,
              FlarmTraffic::AircraftType _aircraft_type =
                  FlarmTraffic::AircraftType::UNKNOWN)
  {
    location = _location;
    altitude = _altitude;
    track = _track;
    speed = _speed;
    climb_rate = _climb_rate;
    turn_rate = _turn_rate;
    aircraft_type = _aircraft_type;
    stamp = std::chrono::steady_clock::now();
  }
};

using OGNTrafficPtr = std::shared_ptr<OGNTraffic>;

/**
 * Helper for boost::geometry::index::rtree.
 */
struct OGNTrafficIndexable {
  typedef GeoPoint result_type;

  [[gnu::pure]]
  result_type operator()(const OGNTrafficPtr &traffic) const
  {
    return traffic->location;
  }
};

class OGNTrafficContainer {
  typedef boost::geometry::index::rtree<
      OGNTrafficPtr, boost::geometry::index::rstar<16>, OGNTrafficIndexable>
      Tree;

  typedef boost::intrusive::list<OGNTraffic,
                                 boost::intrusive::constant_time_size<false> >
      List;

  typedef boost::intrusive::set<
      OGNTraffic, boost::intrusive::compare<OGNTraffic::IdCompare>,
      boost::intrusive::constant_time_size<false> >
      IdSet;

  /**
   * A geospatial container of all OGN traffic.
   */
  Tree rtree;

  /**
   * A linked list of traffic, sorted by last update.
   */
  List list;

  /**
   * Map id to OGNTraffic.
   */
  IdSet id_set;

  /**
   * Map device_id to OGNTraffic.
   */
  std::map<tstring, OGNTrafficPtr> device_map;

  /**
   * The id assigned to the next new OGNTraffic.
   * Start at high IDs to distinguish from XCSoar clients
   */
  unsigned next_id = 0x80000000;

public:
  OGNTrafficContainer();
  ~OGNTrafficContainer();

  void clear();

  bool empty() const { return list.empty(); }

  /**
   * Look up traffic by device ID.
   */
  [[gnu::pure]]
  OGNTraffic *Find(const tstring &device_id);

  /**
   * Look up traffic by internal ID.
   */
  [[gnu::pure]]
  OGNTraffic *FindById(unsigned id);

  /**
   * Create or update OGN traffic.
   */
  OGNTraffic &Make(const tstring &device_id, const GeoPoint &location,
                   int altitude, unsigned track, unsigned speed,
                   int climb_rate, double turn_rate = 0,
                   FlarmTraffic::AircraftType aircraft_type =
                       FlarmTraffic::AircraftType::UNKNOWN);

  void Expire(std::chrono::steady_clock::time_point before);

  typedef Tree::const_query_iterator query_iterator;
  typedef boost::iterator_range<query_iterator> query_iterator_range;

  [[gnu::pure]]
  query_iterator_range QueryWithinRange(GeoPoint location, double range) const;
};
