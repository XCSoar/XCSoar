// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Waypoint.hpp"
#include "util/StaticString.hxx"

#include <vector>
#include <tchar.h>

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
