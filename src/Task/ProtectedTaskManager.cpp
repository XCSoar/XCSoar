/*
Copyright_License {

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

#include "ProtectedTaskManager.hpp"
#include "Task/RoutePlannerGlue.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Points/TaskWaypoint.hpp"
#include "Engine/Route/ReachResult.hpp"

ProtectedTaskManager::ProtectedTaskManager(TaskManager &_task_manager,
                                           const TaskBehaviour &tb)
  :Guard<TaskManager>(_task_manager),
   task_behaviour(tb)
{
}

ProtectedTaskManager::~ProtectedTaskManager() {
  UnprotectedLease lease(*this);
  lease->SetIntersectionTest(nullptr); // de-register
}

void 
ProtectedTaskManager::SetGlidePolar(const GlidePolar &glide_polar)
{
  ExclusiveLease lease(*this);
  lease->SetGlidePolar(glide_polar);
}

const OrderedTaskSettings
ProtectedTaskManager::GetOrderedTaskSettings() const
{
  Lease lease(*this);
  return lease->GetOrderedTask().GetOrderedTaskSettings();
}

WaypointPtr
ProtectedTaskManager::GetActiveWaypoint() const
{
  Lease lease(*this);
  const TaskWaypoint *tp = lease->GetActiveTaskPoint();
  if (tp)
    return tp->GetWaypointPtr();

  return nullptr;
}

bool
ProtectedTaskManager::TargetLock(const unsigned index, bool do_lock)
{
  ExclusiveLease lease(*this);
  return lease->TargetLock(index, do_lock);
}

void 
ProtectedTaskManager::IncrementActiveTaskPoint(int offset)
{
  ExclusiveLease lease(*this);
  lease->IncrementActiveTaskPoint(offset);
}

void 
ProtectedTaskManager::IncrementActiveTaskPointArm(int offset)
{
  ExclusiveLease lease(*this);
  TaskAdvance &advance = lease->SetTaskAdvance();

  switch (advance.GetState()) {
  case TaskAdvance::MANUAL:
  case TaskAdvance::AUTO:
    lease->IncrementActiveTaskPoint(offset);
    break;
  case TaskAdvance::START_DISARMED:
  case TaskAdvance::TURN_DISARMED:
    if (offset>0) {
      advance.SetArmed(true);
    } else {
      lease->IncrementActiveTaskPoint(offset);
    }
    break;
  case TaskAdvance::START_ARMED:
  case TaskAdvance::TURN_ARMED:
    if (offset>0) {
      lease->IncrementActiveTaskPoint(offset);
    } else {
      advance.SetArmed(false);
    }
    break;
  }
}

bool 
ProtectedTaskManager::DoGoto(WaypointPtr &&wp)
{
  ExclusiveLease lease(*this);
  return lease->DoGoto(std::move(wp));
}

OrderedTask*
ProtectedTaskManager::TaskClone() const
{
  Lease lease(*this);
  return lease->Clone(task_behaviour);
}

bool
ProtectedTaskManager::TaskCommit(const OrderedTask& that)
{
  ExclusiveLease lease(*this);
  return lease->Commit(that);
}

void 
ProtectedTaskManager::Reset()
{
  ExclusiveLease lease(*this);
  lease->Reset();
}

void
ProtectedTaskManager::SetRoutePlanner(const RoutePlannerGlue *_route) {
  intersection_test.SetRoute(_route);

  ExclusiveLease lease(*this);
  lease->SetIntersectionTest(&intersection_test);
}

bool
ReachIntersectionTest::Intersects(const AGeoPoint& destination)
{
  if (!route)
    return false;

  ReachResult result;
  if (!route->FindPositiveArrival(destination, result))
    return false;

  // we use find_positive_arrival here instead of is_inside, because may use
  // arrival height for sorting later
  return result.terrain_valid == ReachResult::Validity::UNREACHABLE ||
    (result.terrain_valid == ReachResult::Validity::VALID &&
     result.terrain < destination.altitude);
}

void
ProtectedTaskManager::ResetTask()
{
  ExclusiveLease lease(*this);
  lease->ResetTask();
}
