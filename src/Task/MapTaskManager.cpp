/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Protection.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Interface.hpp"

static const TaskBehaviour&
GetTaskBehaviour()
{
  return XCSoarInterface::GetComputerSettings().task;
}

static MapTaskManager::TaskEditResult
AppendToTask(OrderedTask *task, const Waypoint &waypoint)
{
  if (task->TaskSize()==0)
    return MapTaskManager::NOTASK;

  int i = task->TaskSize() - 1;
  // skip all finish points
  while (i >= 0) {
    const OrderedTaskPoint &tp = task->GetPoint(i);
    if (tp.IsSuccessorAllowed()) {
      ++i;
      break;
    }

    --i;
  }

  const AbstractTaskFactory &factory = task->GetFactory();
  OrderedTaskPoint *task_point =
      (OrderedTaskPoint *)factory.CreateIntermediate(waypoint);
  if (task_point == NULL)
    return MapTaskManager::UNMODIFIED;

  bool success = i >= 0 ? task->Insert(*task_point, i) : task->Append(*task_point);
  delete task_point;

  if (!success)
    return MapTaskManager::UNMODIFIED;

  if (!task->CheckTask())
    return MapTaskManager::INVALID;

  return MapTaskManager::SUCCESS;
}

static MapTaskManager::TaskEditResult
MutateFromGoto(OrderedTask *task, const Waypoint &finish_waypoint,
               const Waypoint &start_waypoint)
{
  const AbstractTaskFactory &factory = task->GetFactory();
  OrderedTaskPoint *start_point =
      (OrderedTaskPoint *)factory.CreateStart(start_waypoint);
  if (start_point == NULL)
    return MapTaskManager::UNMODIFIED;

  bool success = task->Append(*start_point);
  delete start_point;
  if (!success)
    return MapTaskManager::UNMODIFIED;

  OrderedTaskPoint *finish_point =
      (OrderedTaskPoint *)factory.CreateFinish(finish_waypoint);
  if (finish_point == NULL)
    return MapTaskManager::UNMODIFIED;

  success = task->Append(*finish_point);
  delete finish_point;

  if (!success)
    return MapTaskManager::UNMODIFIED;

  return MapTaskManager::MUTATED_FROM_GOTO;
}

MapTaskManager::TaskEditResult
MapTaskManager::AppendToTask(const Waypoint &waypoint)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  TaskEditResult result = MapTaskManager::UNMODIFIED;
  if (task_manager->GetOrderedTask().CheckTask()) {
    OrderedTask *task = task_manager->Clone(GetTaskBehaviour());
    result = AppendToTask(task, waypoint);
    if (result == SUCCESS)
      task_manager->Commit(*task);
    delete task;
  } else { // ordered task invalid
    switch (task_manager->GetMode()) {
    case TaskType::NONE:
    case TaskType::ABORT:
    case TaskType::ORDERED:
      result = task_manager->DoGoto(waypoint) ? MapTaskManager::MUTATED_TO_GOTO :
                              MapTaskManager::UNMODIFIED;
      break;
    case TaskType::GOTO:
    {
      OrderedTask *task = task_manager->Clone(GetTaskBehaviour());
      const TaskWaypoint *OldGotoTWP = task_manager->GetActiveTaskPoint();
      if (!OldGotoTWP)
        break;

      const Waypoint &OldGotoWp = OldGotoTWP->GetWaypoint();
      result = MutateFromGoto(task, waypoint, OldGotoWp);
      if (result == MUTATED_FROM_GOTO)
        task_manager->Commit(*task);

      delete task;
      break;
    }
    }
  }
  return result;
}

static MapTaskManager::TaskEditResult
InsertInTask(OrderedTask *task, const Waypoint &waypoint)
{
  if (task->TaskSize()==0)
    return MapTaskManager::NOTASK;

  int i = task->GetActiveIndex();
  /* skip all start points */
  while (true) {
    if (i >= (int)task->TaskSize())
      return MapTaskManager::UNMODIFIED;

    const OrderedTaskPoint &task_point = task->GetPoint(i);
    if (task_point.IsPredecessorAllowed())
      break;

    ++i;
  }

  const AbstractTaskFactory &factory = task->GetFactory();
  OrderedTaskPoint *task_point =
      (OrderedTaskPoint *)factory.CreateIntermediate(waypoint);
  if (task_point == NULL)
    return MapTaskManager::UNMODIFIED;

  bool success = task->Insert(*task_point, i);
  delete task_point;
  if (!success)
    return MapTaskManager::UNMODIFIED;
  if (!task->CheckTask())
    return MapTaskManager::INVALID;
  return MapTaskManager::SUCCESS;
}

MapTaskManager::TaskEditResult
MapTaskManager::InsertInTask(const Waypoint &waypoint)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  TaskEditResult result = MapTaskManager::UNMODIFIED;
  if (task_manager->GetOrderedTask().CheckTask()) {
    OrderedTask *task = task_manager->Clone(GetTaskBehaviour());

    result = InsertInTask(task, waypoint);
    if (result == SUCCESS)
      task_manager->Commit(*task);
    delete task;
  } else { // ordered task invalid
    switch (task_manager->GetMode()) {
    case TaskType::NONE:
    case TaskType::ABORT:
    case TaskType::ORDERED:
      result = task_manager->DoGoto(waypoint) ? MapTaskManager::MUTATED_TO_GOTO :
                              MapTaskManager::UNMODIFIED;
      break;
    case TaskType::GOTO:
    {
      OrderedTask *task = task_manager->Clone(GetTaskBehaviour());
      const TaskWaypoint *OldGotoTWP = task_manager->GetActiveTaskPoint();
      if (!OldGotoTWP)
        break;
      const Waypoint &OldGotoWp = OldGotoTWP->GetWaypoint();
      result = MutateFromGoto(task, OldGotoWp, waypoint);
      if (result == MUTATED_FROM_GOTO)
        task_manager->Commit(*task);
      delete task;
      break;
    }
    }
  }
  return result;
}

static MapTaskManager::TaskEditResult
ReplaceInTask(OrderedTask *task, const Waypoint &waypoint)
{
  if (task->TaskSize()==0)
    return MapTaskManager::NOTASK;

  unsigned i = task->GetActiveIndex();
  if (i >= task->TaskSize())
    return MapTaskManager::UNMODIFIED;

  task->Relocate(i, waypoint);

  if (!task->CheckTask())
    return MapTaskManager::INVALID;

  return MapTaskManager::SUCCESS;
}

MapTaskManager::TaskEditResult
MapTaskManager::ReplaceInTask(const Waypoint &waypoint)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  OrderedTask *task = task_manager->Clone(GetTaskBehaviour());

  TaskEditResult result = ReplaceInTask(task, waypoint);
  if (result == SUCCESS)
    task_manager->Commit(*task);

  delete task;
  return result;
}

static int
GetIndexInTask(const OrderedTask &task, const Waypoint &waypoint)
{
  if (task.TaskSize() == 0)
    return -1;

  unsigned i = task.GetActiveIndex();
  if (i >= task.TaskSize())
    return -1;

  int TPindex = -1;
  for (unsigned i = task.TaskSize(); i--;) {
    const OrderedTaskPoint &tp = task.GetPoint(i);

    if (tp.GetWaypoint() == waypoint) {
      TPindex = i;
      break;
    }
  }
  return TPindex;
}

int
MapTaskManager::GetIndexInTask(const Waypoint &waypoint)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  if (task_manager->GetMode() == TaskType::ORDERED) {
    const OrderedTask &task = task_manager->GetOrderedTask();
    return GetIndexInTask(task, waypoint);
  }
  return -1;
}

static MapTaskManager::TaskEditResult
RemoveFromTask(OrderedTask *task, const Waypoint &waypoint)
{
  if (task->TaskSize()==0)
    return MapTaskManager::NOTASK;

  int i = GetIndexInTask(*task, waypoint);
  if (i >= 0)
    task->GetFactory().Remove(i);

  // if finish was removed
  if (i == (int)task->TaskSize())
    task->GetFactory().CheckAddFinish();

  if (i == -1)
    return MapTaskManager::UNMODIFIED;

  if (!task->CheckTask())
    return MapTaskManager::INVALID;

  return MapTaskManager::SUCCESS;
}

MapTaskManager::TaskEditResult
MapTaskManager::RemoveFromTask(const Waypoint &wp)
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  OrderedTask *task = task_manager->Clone(GetTaskBehaviour());

  TaskEditResult result = RemoveFromTask(task, wp);
  if (result == SUCCESS)
    task_manager->Commit(*task);

  delete task;
  return result;
}
