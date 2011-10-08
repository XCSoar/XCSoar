/*
Copyright_License {

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

#include "ProtectedTaskManager.hpp"
#include "Util/Serialiser.hpp"
#include "Util/Deserialiser.hpp"
#include "Util/DataNodeXML.hpp"
#include "Task/TaskFile.hpp"
#include "LocalPath.hpp"
#include "Task/RoutePlannerGlue.hpp"

#include <windef.h> // for MAX_PATH

ProtectedTaskManager::~ProtectedTaskManager() {
  ExclusiveLease lease(*this);
  lease->set_intersection_test(NULL); // de-register
}

void 
ProtectedTaskManager::SetGlidePolar(const GlidePolar &glide_polar)
{
  ExclusiveLease lease(*this);
  lease->set_glide_polar(glide_polar);
}

TaskManager::TaskMode_t 
ProtectedTaskManager::GetMode() const
{
  Lease lease(*this);
  return lease->get_mode();
}

const OrderedTaskBehaviour 
ProtectedTaskManager::GetOrderedTaskBehaviour() const
{
  Lease lease(*this);
  return lease->get_ordered_task_behaviour();
}

const Waypoint* 
ProtectedTaskManager::GetActiveWaypoint() const
{
  Lease lease(*this);
  const TaskWaypoint *tp = lease->getActiveTaskPoint();
  if (tp)
    return &tp->GetWaypoint();

  return NULL;
}

bool
ProtectedTaskManager::IsInSector (const unsigned index,
                                  const AircraftState &ref) const
{
  Lease lease(*this);
  return lease->isInSector(index, ref);
}

bool
ProtectedTaskManager::SetTarget(const unsigned index, const fixed range,
   const fixed radial)
{
  ExclusiveLease lease(*this);
  return lease->set_target(index, range, radial);
}

bool
ProtectedTaskManager::TargetLock(const unsigned index, bool do_lock)
{
  ExclusiveLease lease(*this);
  return lease->target_lock(index, do_lock);
}

const TCHAR*
ProtectedTaskManager::GetOrderedTaskpointName(const unsigned index) const
{
 Lease lease(*this);
 return lease->get_ordered_taskpoint_name(index);
}

void 
ProtectedTaskManager::IncrementActiveTaskPoint(int offset)
{
  ExclusiveLease lease(*this);
  lease->incrementActiveTaskPoint(offset);
}

void 
ProtectedTaskManager::IncrementActiveTaskPointArm(int offset)
{
  ExclusiveLease lease(*this);

  switch (lease->get_task_advance().get_advance_state()) {
  case TaskAdvance::MANUAL:
  case TaskAdvance::AUTO:
    lease->incrementActiveTaskPoint(offset);
    break;
  case TaskAdvance::START_DISARMED:
  case TaskAdvance::TURN_DISARMED:
    if (offset>0) {
      lease->get_task_advance().set_armed(true);
    } else {
      lease->incrementActiveTaskPoint(offset);
    }
    break;
  case TaskAdvance::START_ARMED:
  case TaskAdvance::TURN_ARMED:
    if (offset>0) {
      lease->incrementActiveTaskPoint(offset);
    } else {
      lease->get_task_advance().set_armed(false);
    }
    break;
  default:
    assert(1);
  }
}

bool 
ProtectedTaskManager::DoGoto(const Waypoint &wp)
{
  ExclusiveLease lease(*this);
  return lease->do_goto(wp);
}

AircraftState 
ProtectedTaskManager::GetStartState() const
{
  Lease lease(*this);
  return lease->get_start_state();
}

fixed 
ProtectedTaskManager::GetFinishHeight() const
{
  Lease lease(*this);
  return lease->get_finish_height();
}

OrderedTask*
ProtectedTaskManager::TaskClone() const
{
  Lease lease(*this);
  return lease->clone(task_events, task_behaviour,
                      lease->get_glide_polar());
}

OrderedTask* 
ProtectedTaskManager::TaskCopy(const OrderedTask &that) const
{
  Lease lease(*this);
  return that.clone(task_events, task_behaviour, lease->get_glide_polar());
}

OrderedTask* 
ProtectedTaskManager::TaskBlank() const
{
  Lease lease(*this);
  return new OrderedTask(task_events, task_behaviour, lease->get_glide_polar());
}

bool
ProtectedTaskManager::TaskCommit(const OrderedTask& that)
{
  ExclusiveLease lease(*this);
  return lease->commit(that);
}

bool 
ProtectedTaskManager::TaskSave(const TCHAR* path, const OrderedTask& task)
{
  DataNodeXML* root = DataNodeXML::createRoot(_T("Task"));
  Serialiser tser(*root);
  tser.serialise(task);

  bool retval = root->save(path);
  delete root;  
  return retval;
}

bool 
ProtectedTaskManager::TaskSave(const TCHAR* path)
{
  OrderedTask* task = TaskClone();
  bool retval = TaskSave(path, *task);
  delete task;
  return retval;
}

OrderedTask* 
ProtectedTaskManager::TaskCreate(const TCHAR* path, const Waypoints *waypoints,
                                  unsigned index) const
{
  TaskFile* task_file = TaskFile::Create(path);
  if (task_file != NULL) {
    OrderedTask* task = task_file->GetTask(waypoints, index);
    delete task_file;
    return task;
  }
  return NULL;
}

const TCHAR ProtectedTaskManager::default_task_path[] = _T("Default.tsk");


OrderedTask*
ProtectedTaskManager::TaskCreateDefault(const Waypoints *waypoints,
                                          TaskBehaviour::Factory_t factoryfail)
{
  TCHAR path[MAX_PATH];
  LocalPath(path, default_task_path);
  OrderedTask *task = TaskCreate(path, waypoints);
  if (!task) {
    task = TaskBlank();
    assert(task);
    task->set_factory(factoryfail);
  }
  return task;
}

bool 
ProtectedTaskManager::TaskSaveDefault()
{
  TCHAR path[MAX_PATH];
  LocalPath(path, default_task_path);
  return TaskSave(path);
}

void 
ProtectedTaskManager::Reset()
{
  ExclusiveLease lease(*this);
  lease->reset();
}

void
ProtectedTaskManager::SetRoutePlanner(const RoutePlannerGlue *_route) {
  intersection_test.SetRoute(_route);

  ExclusiveLease lease(*this);
  lease->set_intersection_test(&intersection_test);
}

bool
ReachIntersectionTest::Intersects(const AGeoPoint& destination)
{
  if (!route)
    return false;
  RoughAltitude h, h_dummy;
  route->find_positive_arrival(destination, h, h_dummy);
  // we use find_positive_arrival here instead of is_inside, because may use
  // arrival height for sorting later
  return (h< destination.altitude);
}
