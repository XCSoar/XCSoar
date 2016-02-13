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

#include "MatTaskMonitor.hpp"
#include "PageActions.hpp"
#include "Widget/QuestionWidget.hpp"
#include "Form/ActionListener.hpp"
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
#include "Interface.hpp"

class MatTaskAddWidget final
  : public QuestionWidget, private ActionListener {
  MatTaskMonitor &monitor;

  enum Action {
    DISMISS,
    ADD,
  };

  const WaypointPtr waypoint;

  StaticString<256> buffer;

  gcc_pure
  const TCHAR *MakeMessage(const Waypoint &wp) {
    buffer.Format(_T("%s\n%s"), wp.name.c_str(), _("Add this turn point?"));
    return buffer;
  }

public:
  MatTaskAddWidget(MatTaskMonitor &_monitor, WaypointPtr &&_waypoint)
    :QuestionWidget(MakeMessage(*_waypoint), *this),
     monitor(_monitor), waypoint(std::move(_waypoint)) {
    AddButton(_("Add"), ADD);
    AddButton(_("Dismiss"), DISMISS);
  }

  ~MatTaskAddWidget() {
    assert(monitor.widget == this);
    monitor.widget = nullptr;
  }

private:
  void OnAdd();

  /* virtual methods from class ActionListener */
  void OnAction(int id) override;
};

inline void
MatTaskAddWidget::OnAdd()
{
  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  const OrderedTask &task = task_manager->GetOrderedTask();
  const unsigned idx = task.TaskSize() - 1;
  AbstractTaskFactory &factory = task_manager->GetFactory();

  auto wp = waypoint;
  IntermediateTaskPoint *tp =
    factory.CreateIntermediate(TaskPointFactoryType::MAT_CYLINDER,
                               std::move(wp));
  if (tp != nullptr) {
    factory.Insert(*tp, idx, false);
    delete tp;
  }
}

void
MatTaskAddWidget::OnAction(int id)
{
  switch ((Action)id) {
  case DISMISS:
    break;

  case ADD:
    OnAdd();
    break;
  }

  PageActions::RestoreBottom();
}

/**
 * Does this waypoint exist already in the task?  A waypoint must not
 * be added twice to the task.
 */
gcc_pure
static bool
IsInTask(const OrderedTask &task, const Waypoint &wp)
{
  for (unsigned i = 0, n = task.TaskSize(); i < n; ++i)
    if (task.GetTaskPoint(i).GetWaypoint() == wp)
      return true;

  return false;
}

gcc_pure
static bool
IsInOrderedTask(const ProtectedTaskManager &task_manager, const Waypoint &wp)
{
  ProtectedTaskManager::Lease lease(task_manager);
  return IsInTask(lease->GetOrderedTask(), wp);
}

/**
 * Is the current task point the finish point?
 */
gcc_pure
static bool
FinishIsCurrent(const OrderedTask &task)
{
  return task.GetActiveTaskPointIndex() + 1 == task.TaskSize();
}

gcc_pure
static bool
FinishIsCurrent(const ProtectedTaskManager &task_manager)
{
  ProtectedTaskManager::Lease lease(task_manager);
  return FinishIsCurrent(lease->GetOrderedTask());
}

gcc_pure
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
      !stats.start.task_started || stats.task_finished ||
      /* not inside an existing observation zone */
      stats.inside_oz ||
      /* valid GPS fix required to calculate nearest turn point */
      !basic.location_available ||
      /* we must be heading finish, and here we may insert new
         points */
      !FinishIsCurrent(*protected_task_manager))
    /* we only handle MAT tasks */
    return nullptr;

  /* find the nearest turn point within the MAT cylinder standard
     radius */
  auto turnpoint_predicate = [](const Waypoint &wp) {
    return wp.IsTurnpoint();
  };

  auto wp = way_points.GetNearestIf(basic.location,
                                    CylinderZone::MAT_RADIUS,
                                    turnpoint_predicate);

  if (wp == nullptr)
    /* no nearby turn point */
    return nullptr;

  if (IsInOrderedTask(*protected_task_manager, *wp))
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
