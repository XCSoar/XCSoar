/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Retrospective.hpp"
#include "Waypoint/Waypoints.hpp"

Retrospective::Retrospective(const Waypoints &wps)
  :waypoints(wps),
   search_range(15000),
   angle_tolerance(Angle::Degrees(25))
{
}

void
Retrospective::Clear()
{
  candidate_list.clear();
}

void
Retrospective::PruneCandidates()
{
  assert(candidate_list.size()>2);

  auto it_best = candidate_list.end();
  bool erase = false;

  auto it1 = std::next(candidate_list.begin());
  auto it2 = std::next(it1);
  for (; it2 != candidate_list.end(); ++it1, ++it2) {
    if (it1->bearing.CompareRoughly(it2->bearing, angle_tolerance)) {
      it_best = it1;
      erase = true;
    }
  }

  if (erase) {
    it_best = candidate_list.erase(it_best);
    it_best->update_leg(*std::prev(it_best));
  }
}

void
Retrospective::CalcDistances(double &d_ach, double &d_can)
{
  d_ach = 0;
  d_can = 0;
  for (auto it0 = std::next(candidate_list.begin()); it0 != candidate_list.end(); ++it0) {
    d_ach += it0->actual_in;
    d_can += it0->leg_in;
  }
  // last leg part actual_in should be distance from previous to current ac location
}


bool
Retrospective::UpdateSample(const GeoPoint &aircraft_location)
{
  assert(aircraft_location.IsValid());

  // TODO:
  // - look for trivial loops e.g. A-B-A-B-C?
  // - only add candidate if greater distance to previous than tolerance radius
  // -

  // retrospective task

  auto waypoint = waypoints.LookupLocation(aircraft_location, search_range);
  // TODO actually need to find *all* in search range!

  // ignore if none found in search box
  if (!waypoint)
    return false;

  // ignore if none found in actual search range
  if (waypoint->location.Distance(aircraft_location)> search_range)
    return false;

  // initialise with first point found
  if (candidate_list.empty()) {
    candidate_list.emplace_back(std::move(waypoint), aircraft_location);
    return true;
  }

  bool changed = false;

  NearWaypoint &back = candidate_list.back();

  // update current task point if improved
  changed |= back.update_location(aircraft_location);

  if (back.waypoint->id != waypoint->id) {

    // printf("closest to %s\n", waypoint->name.c_str());
    // near new waypoint

    // first check if it's outside range
    auto dist_wpwp = waypoint->location.Distance(back.location);

    if ((dist_wpwp <= search_range) && (candidate_list.size()>1)) {
      // if we have a previous, we can see if this one is a better replacement
      // (replacing it makes a linear collapse of the intermediate point)

      auto previous = std::next(candidate_list.rbegin());

      // distance previous
      auto d_prev_back = previous->location.Distance(back.location);
      auto d_prev_candidate = previous->location.Distance(waypoint->location);

      if (d_prev_candidate > d_prev_back) {
        // replace back with new point
        auto wp2 = waypoint;
        back = NearWaypoint(std::move(wp2), aircraft_location, *previous);
        changed = true;
      }

    }

    if (dist_wpwp > search_range && back.waypoint->id != waypoint->id) {
      // - far enough away (not overlapping) that can consider this a new point
      candidate_list.emplace_back(std::move(waypoint), aircraft_location,
                                  candidate_list.back());
      changed = true;
    }

  }

  if (candidate_list.size()<2) {
    return changed;
  } else if (changed && (candidate_list.size() > 2)) {
    PruneCandidates();
  }

  if (changed) {
    double d_ach = 0;
    double d_can = 0;
    CalcDistances(d_ach, d_can);
  }
  return changed;
}

