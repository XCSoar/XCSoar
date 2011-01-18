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

#include "GotoTask.hpp"
#include "BaseTask/UnorderedTaskPoint.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"

GotoTask::GotoTask(TaskEvents &te, 
                   const TaskBehaviour &tb,
                   const GlidePolar &gp,
                   const Waypoints &wps):
  UnorderedTask(GOTO, te,tb,gp),
  tp(NULL),
  waypoints(wps)
{
}

GotoTask::~GotoTask() 
{
  delete tp;
}

TaskWayPoint*
GotoTask::getActiveTaskPoint() const
{ 
  return tp;
}

bool 
GotoTask::validTaskPoint(const int index_offset) const
{
  if (index_offset !=0) 
    return false;
  return (tp != NULL);
}


void 
GotoTask::setActiveTaskPoint(unsigned index)
{
  // nothing to do
}


bool 
GotoTask::update_sample(const AIRCRAFT_STATE &state,
                        const bool full_update)
{
  return false; // nothing to do
}


bool 
GotoTask::check_transitions(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&)
{
  return false; // nothing to do
}

bool 
GotoTask::do_goto(const Waypoint & wp)
{
  if (task_behaviour.goto_nonlandable || wp.is_landable()) {
    delete tp;
    tp = new UnorderedTaskPoint(wp, task_behaviour);
    return true;
  } else {
    return false;
  }
}

void 
GotoTask::tp_CAccept(TaskPointConstVisitor& visitor, const bool reverse) const
{
  if (tp) {
    visitor.Visit(*tp);
  }
}

unsigned 
GotoTask::task_size() const
{
  if (tp) {
    return 1;
  } else {
    return 0;
  }
}

void
GotoTask::reset()
{
  delete tp;
  UnorderedTask::reset();
}


bool 
GotoTask::check_takeoff(const AIRCRAFT_STATE& state_now, 
                        const AIRCRAFT_STATE& state_last)
{
  if (state_now.Flying && !state_last.Flying && !tp) {
    const Waypoint* wp = waypoints.lookup_location(state_now.Location,
                                                   fixed(1000));
    if (wp) {
      return do_goto(*wp);
    }
  }
  return false;
}
