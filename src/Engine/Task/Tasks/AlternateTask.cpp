/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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


// @todo: call task_events.transition_best_alternate() in update

AlternateTask::AlternateTask(TaskEvents &te, 
                             const TaskBehaviour &tb,
                             GlidePolar &gp,
                             const Waypoints &wps):
  AbortTask(te, tb, gp, wps)
{
}


void
AlternateTask::reset()
{
  AbortTask::reset();
  destination = GeoPoint();
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
AlternateTask::client_update(const AIRCRAFT_STATE &state_now,
                             const bool reachable)
{
  if (!reachable) // for now, only do reachable points
    return;

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

}

void 
AlternateTask::set_task_destination(const AIRCRAFT_STATE &state_now,
                                    const TaskPoint* _target) 
{
  if (_target) {
    destination = _target->get_location_remaining();
  } else {
    destination = state_now.get_location();
  }
}
