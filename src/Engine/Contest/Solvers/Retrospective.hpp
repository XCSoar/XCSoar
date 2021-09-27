/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#ifndef RETROSPECTIVE_HPP
#define RETROSPECTIVE_HPP

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

#endif
