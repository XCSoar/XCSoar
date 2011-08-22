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

#include "Task/MapTaskManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "Engine/Task/TaskEvents.hpp"
#include "Dialogs/Internal.hpp"
#include "Protection.hpp"


MapTaskManager::task_edit_result
MapTaskManager::append_to_task(OrderedTask *task, const Waypoint &wp)
{
  if (task->TaskSize()==0)
    return NOTASK;

  int i = task->TaskSize() - 1;
  // skip all finish points
  while (i >= 0) {
    const OrderedTaskPoint *tp = task->get_tp(i);
    if (tp == NULL)
      break;

    if (tp->successor_allowed()) {
      ++i;
      break;
    }

    --i;
  }

  const AbstractTaskFactory &factory = task->get_factory();
  OrderedTaskPoint *tp = (OrderedTaskPoint *)factory.createIntermediate(wp);
  if (tp == NULL)
    return UNMODIFIED;

  bool success = i >= 0 ? task->insert(*tp, i) : task->append(*tp);
  delete tp;

  if (!success)
    return UNMODIFIED;

  if (!task->check_task())
    return INVALID;

  return SUCCESS;
}

bool
MapTaskManager::do_goto(const Waypoint&wp)
{
  assert(protected_task_manager != NULL);
  return protected_task_manager->do_goto(wp);
}

MapTaskManager::task_edit_result
MapTaskManager::append_to_task(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  TaskEvents task_events;
  GlidePolar glide_polar(task_manager->get_glide_polar());
  OrderedTask *task = task_manager->clone(task_events,
                                          XCSoarInterface::SettingsComputer().task,
                                          glide_polar);

  task_edit_result result = append_to_task(task, wp);
  if (result == SUCCESS)
    task_manager->commit(*task);

  delete task;
  return result;
}

MapTaskManager::task_edit_result
MapTaskManager::insert_in_task(OrderedTask *task, const Waypoint &wp)
{
  if (task->TaskSize()==0)
    return NOTASK;

  int i = task->getActiveIndex();
  /* skip all start points */
  while (true) {
    if (i >= (int)task->TaskSize())
      return UNMODIFIED;

    const TaskPoint *tp = task->get_tp(i);
    if (tp == NULL || tp->IsIntermediatePoint())
      break;

    ++i;
  }

  const AbstractTaskFactory &factory = task->get_factory();
  OrderedTaskPoint *tp = (OrderedTaskPoint *)factory.createIntermediate(wp);
  if (tp == NULL)
    return UNMODIFIED;

  bool success = task->insert(*tp, i);
  delete tp;
  if (!success)
    return UNMODIFIED;

  if (!task->check_task())
    return INVALID;

  return SUCCESS;
}

MapTaskManager::task_edit_result
MapTaskManager::insert_in_task(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  TaskEvents task_events;
  GlidePolar glide_polar(task_manager->get_glide_polar());
  OrderedTask *task = task_manager->clone(task_events,
                                          XCSoarInterface::SettingsComputer().task,
                                          glide_polar);

  task_edit_result result = insert_in_task(task, wp);
  if (result == SUCCESS)
    task_manager->commit(*task);

  delete task;
  return result;
}

MapTaskManager::task_edit_result
MapTaskManager::replace_in_task(OrderedTask *task, const Waypoint &wp)
{
  { // this must be done in thread lock because it potentially changes the
    // waypoints database
    ScopeSuspendAllThreads suspend;
    task->check_duplicate_waypoints(way_points);
    way_points.optimise();
  }

  if (task->TaskSize()==0)
    return NOTASK;

  unsigned i = task->getActiveIndex();
  if (i >= task->TaskSize())
    return UNMODIFIED;

  task->relocate(i, wp);

  if (!task->check_task())
    return INVALID;

  return SUCCESS;
}

MapTaskManager::task_edit_result
MapTaskManager::replace_in_task(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  TaskEvents task_events;
  GlidePolar glide_polar(task_manager->get_glide_polar());
  OrderedTask *task = task_manager->clone(task_events,
                                          XCSoarInterface::SettingsComputer().task,
                                          glide_polar);

  task_edit_result result = replace_in_task(task, wp);
  if (result == SUCCESS)
    task_manager->commit(*task);

  delete task;
  return result;
}

MapTaskManager::task_edit_result
MapTaskManager::remove_from_task(OrderedTask *task, const Waypoint &wp)
{
  if (task->TaskSize()==0)
    return NOTASK;

  { // this must be done in thread lock because it potentially changes the
    // waypoints database
    ScopeSuspendAllThreads suspend;
    task->check_duplicate_waypoints(way_points);
    way_points.optimise();
  }

  bool modified = false;
  for (unsigned i = task->TaskSize(); i--;) {
    const OrderedTaskPoint *tp = task->get_tp(i);
    assert(tp != NULL);

    if (tp->GetWaypoint() == wp) {
      task->remove(i);
      modified = true;
    }
  }

  if (!modified)
    return UNMODIFIED;

  if (!task->check_task())
    return INVALID;

  return SUCCESS;
}

MapTaskManager::task_edit_result
MapTaskManager::remove_from_task(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  TaskEvents task_events;
  GlidePolar glide_polar(task_manager->get_glide_polar());
  OrderedTask *task = task_manager->clone(task_events,
                                          XCSoarInterface::SettingsComputer().task,
                                          glide_polar);

  task_edit_result result = remove_from_task(task, wp);
  if (result == SUCCESS)
    task_manager->commit(*task);

  delete task;
  return result;
}

