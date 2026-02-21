// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Waypoint.hpp"
#include "Math/Angle.hpp"
#include "util/StaticString.hxx"

#include <chrono>
#include <vector>

struct LoggerSettings;
struct Plane;
class OrderedTask;
class OrderedTaskPoint;

struct Declaration {
  struct TurnPoint {
    enum Shape {
      CYLINDER,
      SECTOR,
      LINE,
      DAEC_KEYHOLE
    };

    Waypoint waypoint;
    Shape shape;
    unsigned radius;

    /**
     * Half-angle of the observation zone sector.
     * 180 degrees for a cylinder (full circle),
     * 90 degrees for a FAI quarter-circle sector, etc.
     */
    Angle sector_angle = Angle::HalfCircle();

    /**
     * Inner radius for keyhole-type observation zones (meters).
     * Zero if not applicable.
     */
    unsigned inner_radius = 0;

    /**
     * Is this an AAT (Assigned Area Task) point?
     */
    bool is_aat = false;

    TurnPoint(const Waypoint &_waypoint)
      :waypoint(_waypoint), shape(CYLINDER), radius(1500) {}
    TurnPoint(const OrderedTaskPoint &tp);
  };

  StaticString<64> pilot_name;
  StaticString<64> copilot_name;
  StaticString<32> aircraft_type;
  StaticString<32> aircraft_registration;
  StaticString<8> competition_id;
  std::vector<TurnPoint> turnpoints;

  /**
   * Is the overall task an AAT or MAT type?
   */
  bool is_aat_task = false;

  /**
   * Minimum task time for AAT/MAT tasks.
   */
  std::chrono::duration<unsigned> aat_min_time{};

  Declaration(const LoggerSettings &logger_settings, const Plane &plane,
              const OrderedTask* task);

  void Append(const Waypoint &waypoint) {
    turnpoints.push_back(waypoint);
  }

  const Waypoint &GetWaypoint(unsigned i) const {
    return turnpoints[i].waypoint;
  }

  const Waypoint &GetFirstWaypoint() const {
    return turnpoints.front().waypoint;
  }

  const Waypoint &GetLastWaypoint() const {
    return turnpoints.back().waypoint;
  }

  [[gnu::pure]]
  const char *GetName(const unsigned i) const {
    return turnpoints[i].waypoint.name.c_str();
  }

  [[gnu::pure]]
  const char *GetShortName(const unsigned i) const {
    return turnpoints[i].waypoint.shortname.c_str();
  }

  const GeoPoint &GetLocation(const unsigned i) const {
    return turnpoints[i].waypoint.location;
  }

  [[gnu::pure]]
  unsigned Size() const {
    return turnpoints.size();
  }
};
