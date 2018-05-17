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

#include "GotoTask.hpp"
#include "UnorderedTaskPoint.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Waypoint/Waypoints.hpp"

GotoTask::GotoTask(const TaskBehaviour &tb,
                   const Waypoints &wps)
  :UnorderedTask(TaskType::GOTO, tb),
   tp(NULL),
   waypoints(wps)
{
}

void
GotoTask::SetTaskBehaviour(const TaskBehaviour &tb)
{
  UnorderedTask::SetTaskBehaviour(tb);

  if (tp != NULL)
    tp->SetTaskBehaviour(tb);
}

GotoTask::~GotoTask() 
{
  delete tp;
}

TaskWaypoint*
GotoTask::GetActiveTaskPoint() const
{ 
  return tp;
}

bool 
GotoTask::IsValidTaskPoint(const int index_offset) const
{
  return (index_offset == 0 && tp != NULL);
}


void 
GotoTask::SetActiveTaskPoint(unsigned index)
{
  // nothing to do
}


bool 
GotoTask::UpdateSample(gcc_unused const AircraftState &state,
                       gcc_unused const GlidePolar &glide_polar,
                        gcc_unused const bool full_update)
{
  return false; // nothing to do
}


bool 
GotoTask::DoGoto(WaypointPtr &&wp)
{
  if (task_behaviour.goto_nonlandable || wp->IsLandable()) {
    delete tp;
    tp = new UnorderedTaskPoint(std::move(wp), task_behaviour);
    stats.start.Reset();
    force_full_update = true;
    return true;
  } else {
    return false;
  }
}

void 
GotoTask::AcceptTaskPointVisitor(TaskPointConstVisitor& visitor) const
{
  if (tp)
    visitor.Visit(*tp);
}

unsigned 
GotoTask::TaskSize() const
{
  return tp ? 1 : 0;
}

bool 
GotoTask::TakeoffAutotask(const GeoPoint& location, const double terrain_alt)
{
  if (tp)
    return false;

  auto wp = waypoints.GetNearestLandable(location, 5000);
  if (!wp)
    wp.reset(new Waypoint(waypoints.GenerateTakeoffPoint(location,
                                                         terrain_alt)));

  return DoGoto(std::move(wp));
}
