/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#ifndef RETROSPECTIVE_H
#define RETROSPECTIVE_H

class Waypoints;

#include "Geo/GeoPoint.hpp"
#include "Waypoint/Waypoint.hpp"
#include <list>

class Retrospective {
public:
  Retrospective(const Waypoints &wps);

  struct NearWaypoint {
    Waypoint waypoint;
    GeoPoint location;
    fixed range;
    fixed leg_in;
    fixed actual_in;

    NearWaypoint(const Waypoint &_waypoint, const GeoPoint& _location)
      :waypoint(_waypoint), location(_location), leg_in(0), actual_in(0) {
      range = location.Distance(waypoint.location);
    }

    NearWaypoint(const Waypoint &_waypoint, const GeoPoint& _location,
		 const NearWaypoint& previous):
      waypoint(_waypoint), location(_location) {
      range = location.Distance(waypoint.location);
      update_leg(previous);
    }

    bool update_location(const GeoPoint &location_now) {
      fixed range_now = location_now.Distance(waypoint.location);
      if (range_now < range) {
	range = range_now;
	location = location_now;
	return true;
      }
      return false;
      // TODO: or if distance from previous tp to here is greater than leg (and wasnt previously)
    }
    void update_leg(const NearWaypoint& previous) {
      leg_in = previous.waypoint.location.Distance(waypoint.location);
      actual_in = previous.location.Distance(location);
    }
  };

  typedef std::list<NearWaypoint> NearWaypointList;

protected:
  const Waypoints &waypoints;

  void CalcDistances(fixed& d_ach, fixed& d_can);

  NearWaypointList candidate_list;

  void PruneCandidates();

public:
  const NearWaypointList& getNearWaypointList() const {
    return candidate_list;
  }

  bool UpdateSample(const GeoPoint &aircraft_location);
  void Clear();
  void Reset() { 
    Clear();
  }
};

#endif
