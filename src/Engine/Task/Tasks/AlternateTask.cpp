/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "AlternateTask.hpp"

#include "BaseTask/TaskWaypoint.hpp"
#include "Math/Earth.hpp"
#include "Task/TaskEvents.hpp"
#include "Util/queue.hpp"

#include <limits.h>

const unsigned AlternateTask::max_alternates = 6;

AlternateTask::AlternateTask(TaskEvents &te, const TaskBehaviour &tb,
                             const GlidePolar &gp, const Waypoints &wps):
  AbortTask(te, tb, gp, wps),
  best_alternate_id(UINT_MAX)
{
  alternates.reserve(64);
}

void
AlternateTask::reset()
{
  AbortTask::reset();
  destination = GeoPoint(Angle::zero(), Angle::zero());
  best_alternate_id = UINT_MAX;
}

void
AlternateTask::clear()
{
  AbortTask::clear();
  alternates.clear();
}

/**
 * Function object used to rank waypoints by arrival time
 */
struct AlternateRank:
  public std::binary_function<AlternateTask::Divert,
                              AlternateTask::Divert, bool>
{
  /**
   * Condition, ranks by distance diversion 
   */
  bool operator()(const AlternateTask::Divert& x, 
                  const AlternateTask::Divert& y) const {
    return x.delta < y.delta;
  }
};

void
AlternateTask::check_alternate_changed()
{
  // remember previous value (or invalid)
  unsigned id = (alternates.empty() ? UINT_MAX : alternates[0].waypoint.id);

  // send notification on change
  if (best_alternate_id != id) {
    best_alternate_id = id;
    task_events.transition_alternate();
  }
}

void 
AlternateTask::client_update(const AircraftState &state_now,
                             const bool reachable)
{
  // build a list of alternates, sorted by distance.
  // this is done in separate stages so we can add the reachable ones
  // before the unreachable ones, without the sort criteria affecting
  // the reachability.

  reservable_priority_queue<Divert, DivertVector, AlternateRank> q;
  q.reserve(task_points.size());

  const fixed dist_straight = state_now.location.distance(destination);

  const AlternateTaskVector::const_iterator end = task_points.end();
  for (AlternateTaskVector::const_iterator i = task_points.begin(); i != end; ++i) {
    const TaskWaypoint& tp = *i;
    const Waypoint& wp_alt = tp.GetWaypoint();
    const fixed dist_divert =
        ::DoubleDistance(state_now.location, wp_alt.location, destination);
    const fixed delta = dist_straight - dist_divert;

    q.push(Divert(wp_alt, i->solution, delta));
  }

  // now push results onto the list, best first.
  while (!q.empty() && alternates.size() < max_alternates) {
    const Alternate &top = q.top();

    // only add if not already in the list (from previous stage in two
    // stage process)
    if (!is_waypoint_in_alternates(top.waypoint)) {
      alternates.push_back(top);
    }

    q.pop();
  }

  // check for notifications
  check_alternate_changed();
}

void 
AlternateTask::set_task_destination(const AircraftState &state_now,
                                    const TaskPoint* _target) 
{
  // if we have a target, use that, otherwise use the aircraft location
  // (which ends up equivalent to sorting by distance)
  if (_target)
    destination = _target->GetLocationRemaining();
  else
    destination = state_now.location;
}

void
AlternateTask::set_task_destination_home(const AircraftState &state_now)
{
  // if we have a home, use that, otherwise use the aircraft location
  // (which ends up equivalent to sorting by distance)
  const Waypoint *home_waypoint = waypoints.GetHome();
  if (home_waypoint)
    destination = home_waypoint->location;
  else
    destination = state_now.location;
}

bool 
AlternateTask::is_waypoint_in_alternates(const Waypoint& waypoint) const
{
  const AlternateVector::const_iterator end = alternates.end();
  for (AlternateVector::const_iterator it = alternates.begin();
       it != end; ++it)
    if (it->waypoint == waypoint)
      return true;

  return false;
}
