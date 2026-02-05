// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MatTaskMonitor.hpp"
#include "PageActions.hpp"
#include "Widget/QuestionWidget.hpp"
#include "Language/Language.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/IntermediatePoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Interface.hpp"

class MatTaskAddWidget final
  : public QuestionWidget
{
  MatTaskMonitor &monitor;

  const WaypointPtr waypoint;

  StaticString<256> buffer;

  [[gnu::pure]]
  const char *MakeMessage(const Waypoint &wp) {
    buffer.Format(_T("%s\n%s"), wp.name.c_str(), _("Add this turn point?"));
    return buffer;
  }

public:
  MatTaskAddWidget(MatTaskMonitor &_monitor, WaypointPtr &&_waypoint)
    :QuestionWidget(MakeMessage(*_waypoint)),
     monitor(_monitor), waypoint(std::move(_waypoint)) {
    AddButton(_("Add"), [this](){
      OnAdd();
      PageActions::RestoreBottom();
    });
    AddButton(_("Dismiss"), [](){
      PageActions::RestoreBottom();
    });
  }

  ~MatTaskAddWidget() {
    assert(monitor.widget == this);
    monitor.widget = nullptr;
  }

private:
  void OnAdd();
};

inline void
MatTaskAddWidget::OnAdd()
{
  ProtectedTaskManager::ExclusiveLease task_manager{*backend_components->protected_task_manager};
  const OrderedTask &task = task_manager->GetOrderedTask();
  const unsigned idx = task.TaskSize() - 1;
  AbstractTaskFactory &factory = task_manager->GetFactory();

  auto wp = waypoint;
  auto tp =
    factory.CreateIntermediate(TaskPointFactoryType::MAT_CYLINDER,
                               std::move(wp));
  if (tp != nullptr) {
    factory.Insert(*tp, idx, false);
  }
}

/**
 * Does this waypoint exist already in the task?  A waypoint must not
 * be added twice to the task.
 */
[[gnu::pure]]
static bool
IsInTask(const OrderedTask &task, const Waypoint &wp)
{
  for (unsigned i = 0, n = task.TaskSize(); i < n; ++i)
    if (task.GetTaskPoint(i).GetWaypoint() == wp)
      return true;

  return false;
}

[[gnu::pure]]
static bool
IsInOrderedTask(const ProtectedTaskManager &task_manager, const Waypoint &wp)
{
  ProtectedTaskManager::Lease lease(task_manager);
  return IsInTask(lease->GetOrderedTask(), wp);
}

/**
 * Is the current task point the finish point?
 */
[[gnu::pure]]
static bool
FinishIsCurrent(const OrderedTask &task)
{
  return task.GetActiveTaskPointIndex() + 1 == task.TaskSize();
}

[[gnu::pure]]
static bool
FinishIsCurrent(const ProtectedTaskManager &task_manager)
{
  ProtectedTaskManager::Lease lease(task_manager);
  return FinishIsCurrent(lease->GetOrderedTask());
}

[[gnu::pure]]
static WaypointPtr
FindMatTurnpoint()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const CommonStats &common_stats = calculated.common_stats;
  const TaskStats &stats = calculated.ordered_task_stats;
  if (common_stats.task_type != TaskType::ORDERED ||
      /* require a valid MAT task */
      !stats.task_valid || !stats.is_mat ||
      /* task must be started already, but not finished */
      !stats.start.HasStarted() || stats.task_finished ||
      /* not inside an existing observation zone */
      stats.inside_oz ||
      /* valid GPS fix required to calculate nearest turn point */
      !basic.location_available ||
      /* we must be heading finish, and here we may insert new
         points */
      !FinishIsCurrent(*backend_components->protected_task_manager))
    /* we only handle MAT tasks */
    return nullptr;

  /* find the nearest turn point within the MAT cylinder standard
     radius */
  auto turnpoint_predicate = [](const Waypoint &wp) {
    return wp.IsTurnpoint();
  };

  auto wp = data_components->waypoints->GetNearestIf(basic.location,
                                                     CylinderZone::MAT_RADIUS,
                                                     turnpoint_predicate);

  if (wp == nullptr)
    /* no nearby turn point */
    return nullptr;

  if (IsInOrderedTask(*backend_components->protected_task_manager, *wp))
    /* already in task */
    return nullptr;

  return wp;
}

void
MatTaskMonitor::Check()
{
  auto wp = FindMatTurnpoint();
  if (wp != nullptr) {
    /* found a turn point: open a QuestionWidget (unless one already
       exists) */
    const auto id = wp->id;
    if (widget == nullptr && wp->id != last_id) {
      widget = new MatTaskAddWidget(*this, std::move(wp));
      PageActions::SetCustomBottom(widget);
    }

    last_id = id;
  } else {
    /* no nearby turn point: close the QuestionWidget */
    if (widget != nullptr)
      PageActions::RestoreBottom();

    last_id = unsigned(-1);
  }
}
