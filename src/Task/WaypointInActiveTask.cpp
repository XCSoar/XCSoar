// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointInActiveTask.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Engine/Task/AbstractTask.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Task/ProtectedTaskManager.hpp"

bool
WaypointPtrInActiveOrderedTask(WaypointPtr wp) noexcept
{
  if (!wp || backend_components == nullptr ||
      backend_components->protected_task_manager == nullptr)
    return false;

  ProtectedTaskManager::Lease lease(*backend_components->protected_task_manager);
  const AbstractTask *at = lease->GetActiveTask();
  if (at == nullptr || at->GetType() != TaskType::ORDERED)
    return false;

  const OrderedTask &ot = static_cast<const OrderedTask &>(*at);
  for (unsigned i = 0; i < ot.TaskSize(); ++i) {
    const WaypointPtr &tw = ot.GetTaskPoint(i).GetWaypointPtr();
    if (tw.get() == wp.get())
      return true;
  }

  return false;
}
