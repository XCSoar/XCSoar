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

#include "AlternateTask.hpp"
#include "Task/Points/TaskWaypoint.hpp"
#include "Geo/Math.hpp"
#include "Task/TaskEvents.hpp"
#include "Util/ReservablePriorityQueue.hpp"
#include "Navigation/Aircraft.hpp"

#include <limits.h>

const unsigned AlternateTask::max_alternates = 6;

AlternateTask::AlternateTask(const TaskBehaviour &tb,
                             const Waypoints &wps):
  AbortTask(tb, wps),
  best_alternate_id(UINT_MAX)
{
  alternates.reserve(64);
}

void
AlternateTask::Reset()
{
  AbortTask::Reset();
  destination.SetInvalid();
  best_alternate_id = UINT_MAX;
}

void
AlternateTask::Clear()
{
  AbortTask::Clear();
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
AlternateTask::CheckAlternateChanged()
{
  // remember previous value (or invalid)
  unsigned id = (alternates.empty() ? UINT_MAX : alternates[0].waypoint.id);

  // send notification on change
  if (best_alternate_id != id) {
    best_alternate_id = id;

    if (task_events != NULL)
      task_events->AlternateTransition();
  }
}

void 
AlternateTask::ClientUpdate(const AircraftState &state_now,
                             const bool reachable)
{
  // build a list of alternates, sorted by distance.
  // this is done in separate stages so we can add the reachable ones
  // before the unreachable ones, without the sort criteria affecting
  // the reachability.

  if (!destination.IsValid())
    return;

  reservable_priority_queue<Divert, DivertVector, AlternateRank> q;
  q.reserve(task_points.size());

  const fixed straight_distance = state_now.location.Distance(destination);

  for (auto i = task_points.begin(), end = task_points.end(); i != end; ++i) {
    const TaskWaypoint &tp = *i;
    const Waypoint &wp = tp.GetWaypoint();

    const fixed diversion_distance =
        ::DoubleDistance(state_now.location, wp.location, destination);
    const fixed delta = straight_distance - diversion_distance;

    q.push(Divert(wp, i->solution, delta));
  }

  // now push results onto the list, best first.
  while (!q.empty() && alternates.size() < max_alternates) {
    const Alternate &top = q.top();

    // only add if not already in the list (from previous stage in two
    // stage process)
    if (!IsWaypointInAlternates(top.waypoint))
      alternates.push_back(top);

    q.pop();
  }

  // check for notifications
  CheckAlternateChanged();
}

void 
AlternateTask::SetTaskDestination(const GeoPoint &_destination)
{
  destination = _destination;
}

bool 
AlternateTask::IsWaypointInAlternates(const Waypoint& waypoint) const
{
  for (auto it = alternates.begin(), end = alternates.end(); it != end; ++it)
    if (it->waypoint == waypoint)
      return true;

  return false;
}
