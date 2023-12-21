// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProtectedTaskManager.hpp"
#include "ProtectedRoutePlanner.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/Points/TaskWaypoint.hpp"
#include "Engine/Route/ReachResult.hpp"

ProtectedTaskManager::ProtectedTaskManager(TaskManager &_task_manager,
                                           const TaskBehaviour &tb) noexcept
  :Guard<TaskManager>(_task_manager),
   task_behaviour(tb)
{
}

ProtectedTaskManager::~ProtectedTaskManager() noexcept
{
  UnprotectedLease lease(*this);
  lease->SetIntersectionTest(nullptr); // de-register
}

void 
ProtectedTaskManager::SetGlidePolar(const GlidePolar &glide_polar) noexcept
{
  ExclusiveLease lease(*this);
  lease->SetGlidePolar(glide_polar);
}

void
ProtectedTaskManager::SetStartTimeSpan(const RoughTimeSpan &open_time_span) noexcept
{
  ExclusiveLease lease(*this);
  OrderedTaskSettings otb = lease->GetOrderedTask().GetOrderedTaskSettings();
  otb.start_constraints.open_time_span = open_time_span;
  lease->SetOrderedTaskSettings(otb);
}

const OrderedTaskSettings
ProtectedTaskManager::GetOrderedTaskSettings() const noexcept
{
  Lease lease(*this);
  return lease->GetOrderedTask().GetOrderedTaskSettings();
}

WaypointPtr
ProtectedTaskManager::GetActiveWaypoint() const noexcept
{
  Lease lease(*this);
  const auto *task = lease->GetActiveTask();
  if (task == nullptr)
    return nullptr;

  const TaskWaypoint *tp = task->GetActiveTaskPoint();
  if (tp)
    return tp->GetWaypointPtr();

  return nullptr;
}

bool
ProtectedTaskManager::TargetLock(const unsigned index, bool do_lock) noexcept
{
  ExclusiveLease lease(*this);
  return lease->TargetLock(index, do_lock);
}

void 
ProtectedTaskManager::IncrementActiveTaskPoint(int offset) noexcept
{
  ExclusiveLease lease(*this);
  lease->IncrementActiveTaskPoint(offset);
}

void 
ProtectedTaskManager::IncrementActiveTaskPointArm(int offset) noexcept
{
  ExclusiveLease lease(*this);
  TaskAdvance &advance = lease->SetTaskAdvance();
  OrderedTaskPoint *nextwp = nullptr;

  const auto &ordered_task = lease->GetOrderedTask();

  switch (advance.GetState()) {
  case TaskAdvance::MANUAL:
  case TaskAdvance::AUTO:
    lease->IncrementActiveTaskPoint(offset);
    nextwp = static_cast<OrderedTaskPoint *>(ordered_task.GetActiveTaskPoint());
    break;
  case TaskAdvance::START_DISARMED:
  case TaskAdvance::TURN_DISARMED:
    if (offset>0) {
      advance.SetArmed(true);
    } else {
      lease->IncrementActiveTaskPoint(offset);
      nextwp = static_cast<OrderedTaskPoint *>(ordered_task.GetActiveTaskPoint());
    }
    break;
  case TaskAdvance::START_ARMED:
  case TaskAdvance::TURN_ARMED:
    if (offset>0) {
      lease->IncrementActiveTaskPoint(offset);
      nextwp = static_cast<OrderedTaskPoint *>(ordered_task.GetActiveTaskPoint());
    } else {
      advance.SetArmed(false);
    }
    break;
  }

  // forget that we have visited that waypoint already
  if(nextwp && nextwp->HasEntered()) nextwp->Reset();
}

bool 
ProtectedTaskManager::DoGoto(WaypointPtr &&wp) noexcept
{
  ExclusiveLease lease(*this);
  return lease->DoGoto(std::move(wp));
}

std::unique_ptr<OrderedTask>
ProtectedTaskManager::TaskClone() const noexcept
{
  Lease lease(*this);
  return lease->Clone(task_behaviour);
}

bool
ProtectedTaskManager::TaskCommit(const OrderedTask &that) noexcept
{
  ExclusiveLease lease(*this);
  return lease->Commit(that);
}

void 
ProtectedTaskManager::Reset() noexcept
{
  ExclusiveLease lease(*this);
  lease->Reset();
}

void
ProtectedTaskManager::SetRoutePlanner(const ProtectedRoutePlanner *_route) noexcept
{
  intersection_test.SetRoute(_route);

  ExclusiveLease lease(*this);
  lease->SetIntersectionTest(&intersection_test);
}

bool
ReachIntersectionTest::Intersects(const AGeoPoint &destination) const noexcept
{
  if (!route)
    return false;

  const auto result = route->FindPositiveArrival(destination);
  if (!result)
    return false;

  // we use find_positive_arrival here instead of is_inside, because may use
  // arrival height for sorting later
  return result->terrain_valid == ReachResult::Validity::UNREACHABLE ||
    (result->terrain_valid == ReachResult::Validity::VALID &&
     result->terrain < destination.altitude);
}

void
ProtectedTaskManager::ResetTask() noexcept
{
  ExclusiveLease lease(*this);
  lease->ResetTask();
}
