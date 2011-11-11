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

static const TaskBehaviour&
GetTaskBehaviour()
{
  return XCSoarInterface::SettingsComputer().task;
}

static MapTaskManager::task_edit_result
append_to_task(OrderedTask *task, const Waypoint &wp)
{
  if (task->TaskSize()==0)
    return MapTaskManager::NOTASK;

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

  const AbstractTaskFactory &factory = task->GetFactory();
  OrderedTaskPoint *tp = (OrderedTaskPoint *)factory.createIntermediate(wp);
  if (tp == NULL)
    return MapTaskManager::UNMODIFIED;

  bool success = i >= 0 ? task->Insert(*tp, i) : task->Append(*tp);
  delete tp;

  if (!success)
    return MapTaskManager::UNMODIFIED;

  if (!task->CheckTask())
    return MapTaskManager::INVALID;

  return MapTaskManager::SUCCESS;
}

static MapTaskManager::task_edit_result
mutate_from_goto(OrderedTask *task, const Waypoint &WPFinish,
                                 const Waypoint &WPStart)
{
  const AbstractTaskFactory &factory = task->GetFactory();
  OrderedTaskPoint *sp = (OrderedTaskPoint *)factory.createStart(WPStart);
  if (sp == NULL)
    return MapTaskManager::UNMODIFIED;

  bool success = task->Append(*sp);
  delete sp;
  if (!success)
    return MapTaskManager::UNMODIFIED;

  OrderedTaskPoint *fp = (OrderedTaskPoint *)factory.createFinish(WPFinish);
  if (fp == NULL)
    return MapTaskManager::UNMODIFIED;

  success = task->Append(*fp);
  delete fp;

  if (!success)
    return MapTaskManager::UNMODIFIED;

  return MapTaskManager::MUTATED_FROM_GOTO;
}

MapTaskManager::task_edit_result
MapTaskManager::append_to_task(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  TaskEvents task_events;
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  task_edit_result result = MapTaskManager::UNMODIFIED;
  if (task_manager->GetOrderedTask().CheckTask()) {
    OrderedTask *task = task_manager->Clone(task_events,
                                            GetTaskBehaviour(),
                                            task_manager->GetGlidePolar());
    result = append_to_task(task, wp);
    if (result == SUCCESS)
      task_manager->Commit(*task);
    delete task;
  } else { // ordered task invalid
    switch (task_manager->GetMode()) {
    case TaskManager::MODE_NULL:
    case TaskManager::MODE_ABORT:
    case TaskManager::MODE_ORDERED:
      result = task_manager->DoGoto(wp) ? MapTaskManager::MUTATED_TO_GOTO :
                              MapTaskManager::UNMODIFIED;
      break;
    case TaskManager::MODE_GOTO:
    {
      OrderedTask *task = task_manager->Clone(task_events,
                                              GetTaskBehaviour(),
                                              task_manager->GetGlidePolar());
      const TaskWaypoint *OldGotoTWP = task_manager->GetActiveTaskPoint();
      if (!OldGotoTWP)
        break;

      const Waypoint &OldGotoWp = OldGotoTWP->GetWaypoint();
      result = mutate_from_goto(task, wp, OldGotoWp);
      if (result == MUTATED_FROM_GOTO)
        task_manager->Commit(*task);

      delete task;
      break;
    }
    default:
      break;
    }
  }
  return result;
}

static MapTaskManager::task_edit_result
insert_in_task(OrderedTask *task, const Waypoint &wp)
{
  if (task->TaskSize()==0)
    return MapTaskManager::NOTASK;

  int i = task->GetActiveIndex();
  /* skip all start points */
  while (true) {
    if (i >= (int)task->TaskSize())
      return MapTaskManager::UNMODIFIED;

    const OrderedTaskPoint *tp = task->get_tp(i);
    if (tp == NULL || tp->predecessor_allowed())
      break;

    ++i;
  }

  const AbstractTaskFactory &factory = task->GetFactory();
  OrderedTaskPoint *tp = (OrderedTaskPoint *)factory.createIntermediate(wp);
  if (tp == NULL)
    return MapTaskManager::UNMODIFIED;

  bool success = task->Insert(*tp, i);
  delete tp;
  if (!success)
    return MapTaskManager::UNMODIFIED;
  if (!task->CheckTask())
    return MapTaskManager::INVALID;
  return MapTaskManager::SUCCESS;
}

MapTaskManager::task_edit_result
MapTaskManager::insert_in_task(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  TaskEvents task_events;
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  task_edit_result result = MapTaskManager::UNMODIFIED;
  if (task_manager->GetOrderedTask().CheckTask()) {
    OrderedTask *task = task_manager->Clone(task_events,
                                            GetTaskBehaviour(),
                                            task_manager->GetGlidePolar());

    result = insert_in_task(task, wp);
    if (result == SUCCESS)
      task_manager->Commit(*task);
    delete task;
  } else { // ordered task invalid
    switch (task_manager->GetMode()) {
    case TaskManager::MODE_NULL:
    case TaskManager::MODE_ABORT:
    case TaskManager::MODE_ORDERED:
      result = task_manager->DoGoto(wp) ? MapTaskManager::MUTATED_TO_GOTO :
                              MapTaskManager::UNMODIFIED;
      break;
    case TaskManager::MODE_GOTO:
    {
      OrderedTask *task = task_manager->Clone(task_events,
                                              GetTaskBehaviour(),
                                              task_manager->GetGlidePolar());
      const TaskWaypoint *OldGotoTWP = task_manager->GetActiveTaskPoint();
      if (!OldGotoTWP)
        break;
      const Waypoint &OldGotoWp = OldGotoTWP->GetWaypoint();
      result = mutate_from_goto(task, OldGotoWp, wp);
      if (result == MUTATED_FROM_GOTO)
        task_manager->Commit(*task);
      delete task;
      break;
    }
    default:
      break;
    }
  }
  return result;
}

static MapTaskManager::task_edit_result
replace_in_task(OrderedTask *task, const Waypoint &wp)
{
  if (task->TaskSize()==0)
    return MapTaskManager::NOTASK;

  unsigned i = task->GetActiveIndex();
  if (i >= task->TaskSize())
    return MapTaskManager::UNMODIFIED;

  task->Relocate(i, wp);

  if (!task->CheckTask())
    return MapTaskManager::INVALID;

  return MapTaskManager::SUCCESS;
}

MapTaskManager::task_edit_result
MapTaskManager::replace_in_task(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  TaskEvents task_events;
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  OrderedTask *task = task_manager->Clone(task_events,
                                          GetTaskBehaviour(),
                                          task_manager->GetGlidePolar());

  task_edit_result result = replace_in_task(task, wp);
  if (result == SUCCESS)
    task_manager->Commit(*task);

  delete task;
  return result;
}

static int
index_of_point_in_task(const OrderedTask &task, const Waypoint &wp)
{
  if (task.TaskSize() == 0)
    return -1;

  unsigned i = task.GetActiveIndex();
  if (i >= task.TaskSize())
    return -1;

  int TPindex = -1;
  for (unsigned i = task.TaskSize(); i--;) {
    const OrderedTaskPoint &tp = task.GetPoint(i);

    if (tp.GetWaypoint() == wp) {
      TPindex = i;
      break;
    }
  }
  return TPindex;
}

int
MapTaskManager::index_of_point_in_task(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  if (task_manager->GetMode() == TaskManager::MODE_ORDERED) {
    const OrderedTask &task = task_manager->GetOrderedTask();
    return index_of_point_in_task(task, wp);
  }
  return -1;
}

static MapTaskManager::task_edit_result
remove_from_task(OrderedTask *task, const Waypoint &wp)
{
  if (task->TaskSize()==0)
    return MapTaskManager::NOTASK;

  int TPIndex = index_of_point_in_task(*task, wp);
  if (TPIndex >= 0)
    task->GetFactory().remove(TPIndex);

  // if finish was removed
  if (TPIndex == (int)task->TaskSize())
    task->GetFactory().CheckAddFinish();

  if (TPIndex == -1)
    return MapTaskManager::UNMODIFIED;

  if (!task->CheckTask())
    return MapTaskManager::INVALID;

  return MapTaskManager::SUCCESS;
}

MapTaskManager::task_edit_result
MapTaskManager::remove_from_task(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  TaskEvents task_events;
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  OrderedTask *task = task_manager->Clone(task_events,
                                          GetTaskBehaviour(),
                                          task_manager->GetGlidePolar());

  task_edit_result result = remove_from_task(task, wp);
  if (result == SUCCESS)
    task_manager->Commit(*task);

  delete task;
  return result;
}
