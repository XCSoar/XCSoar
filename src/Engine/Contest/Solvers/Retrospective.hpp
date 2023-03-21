// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Engine/Waypoint/Ptr.hpp"
#include "Math/Angle.hpp"

#include <list>

class Waypoints;

class Retrospective {
public:
  explicit Retrospective(const Waypoints &wps) noexcept;

  struct NearWaypoint {
    WaypointPtr waypoint;
    GeoPoint location;
    double range;
    double leg_in;
    double actual_in;
    Angle bearing;

    NearWaypoint(WaypointPtr &&_waypoint, const GeoPoint &_location) noexcept;

    NearWaypoint(WaypointPtr &&_waypoint, const GeoPoint &_location,
                 const NearWaypoint &previous) noexcept;

    bool update_location(const GeoPoint &location_now) noexcept;
    void update_leg(const NearWaypoint &previous) noexcept;
  };

  typedef std::list<NearWaypoint> NearWaypointList;

protected:
  const Waypoints &waypoints;

  NearWaypointList candidate_list;

  void PruneCandidates() noexcept;

public:
  const NearWaypointList& getNearWaypointList() const noexcept {
    return candidate_list;
  }

  bool UpdateSample(const GeoPoint &aircraft_location) noexcept;
  void Clear() noexcept;
  void Reset() noexcept {
    Clear();
  }

  void CalcDistances(double &d_ach, double &d_can) noexcept;

  /** search range in m */
  double search_range;
  Angle angle_tolerance;
};
