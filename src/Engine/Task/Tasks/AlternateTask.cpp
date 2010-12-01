/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include <queue>
#include "BaseTask/TaskPoint.hpp"
#include "Math/Earth.hpp"
#include "Task/TaskEvents.hpp"

#include <limits.h>

const unsigned AlternateTask::max_alternates = 3;

AlternateTask::AlternateTask(TaskEvents &te, 
                             const TaskBehaviour &tb,
                             const GlidePolar &gp,
                             const Waypoints &wps):
  AbortTask(te, tb, gp, wps),
  best_alternate_id(UINT_MAX)
{
}


void
AlternateTask::reset()
{
  AbortTask::reset();
  destination = GeoPoint(Angle::native(fixed_zero), Angle::native(fixed_zero));
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
struct AlternateRank : public std::binary_function<AlternateTask::Divert, 
                                                   AlternateTask::Divert, bool> {
  /**
   * Condition, ranks by distance diversion 
   */
  bool operator()(const AlternateTask::Divert& x, 
                  const AlternateTask::Divert& y) const {
    return (x.second < y.second);
  }
};


void
AlternateTask::check_alternate_changed()
{
  // remember previous value (or invalid)
  unsigned id;
  if (alternates.size()) {
    id = alternates[0].first.id;
  } else {
    id = UINT_MAX;
  }
  // send notification on change
  if (best_alternate_id != id) {
    best_alternate_id = id;
    task_events.transition_alternate();
  }
}


void 
AlternateTask::client_update(const AIRCRAFT_STATE &state_now,
                             const bool reachable)
{
  // build a list of alternates, sorted by distance.
  // this is done in separate stages so we can add the reachable ones
  // before the unreachable ones, without the sort criteria affecting
  // the reachability.

  std::priority_queue<Divert, DivertVector, AlternateRank> q;

  for (AlternateTaskVector::const_iterator 
         i= tps.begin(); i!= tps.end(); ++i) {

    const TaskPoint& tp = *i->first;
    const Waypoint& wp_alt = tp.get_waypoint();
    const fixed dist_straight = state_now.get_location().distance(destination);
    const fixed dist_divert = ::DoubleDistance(state_now.get_location(),
                                               wp_alt.Location,
                                               destination);
    const fixed delta = dist_straight-dist_divert;

    q.push(std::make_pair(std::make_pair(wp_alt, i->second), delta));
  }

  // now push results onto the list, best first.
  while (!q.empty() && alternates.size()< max_alternates) {
    const Alternate top = q.top().first;

    // only add if not already in the list (from previous stage in two
    // stage process)
    if (!is_waypoint_in_alternates(top.first)) {
      alternates.push_back(top);
    }

    q.pop();
  }

  // check for notifications
  check_alternate_changed();
}


void 
AlternateTask::set_task_destination(const AIRCRAFT_STATE &state_now,
                                    const TaskPoint* _target) 
{
  // if we have a target, use that, otherwise use the aircraft location
  // (which ends up equivalent to sorting by distance)
  if (_target) {
    destination = _target->get_location_remaining();
  } else {
    destination = state_now.get_location();
  }
}

const AbortTask::AlternateVector
AlternateTask::getAlternates() const
{
  return alternates;
}

bool 
AlternateTask::is_waypoint_in_alternates(const Waypoint& waypoint) const
{
  for (AlternateVector::const_iterator it = alternates.begin();
       it != alternates.end(); ++it) {
    if (it->first == waypoint) {
      return true;
    }
  }
  return false;
}
