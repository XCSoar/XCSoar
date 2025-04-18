// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskAdvanceMonitor.hpp"
#include "PageActions.hpp"
#include "Widget/QuestionWidget.hpp"
#include "Language/Language.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/TaskAdvance.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Interface.hpp"

class TaskAdvanceWidget final
  : public QuestionWidget
{
  TaskAdvanceMonitor &monitor;

public:
  TaskAdvanceWidget(TaskAdvanceMonitor &_monitor)
    :QuestionWidget(_("In sector, arm advance when ready")),
     monitor(_monitor) {
    AddButton(_("Arm"), [](){
      {
        ProtectedTaskManager::ExclusiveLease task_manager(*backend_components->protected_task_manager);
        TaskAdvance &advance = task_manager->SetTaskAdvance();
        advance.SetArmed(true);
      }

      PageActions::RestoreBottom();
    });

    AddButton(_("Dismiss"), [](){
      PageActions::RestoreBottom();
    });
  }

  ~TaskAdvanceWidget() {
    assert(monitor.widget == this);
    monitor.widget = nullptr;
  }
};

void
TaskAdvanceMonitor::Check()
{
  const TaskStats &stats = CommonInterface::Calculated().ordered_task_stats;

  if (stats.need_to_arm) {
    if (widget == nullptr && (!last_need_to_arm ||
                              stats.active_index != last_active_index)) {
      widget = new TaskAdvanceWidget(*this);
      PageActions::SetCustomBottom(widget);
    }
  } else {
    if (widget != nullptr)
      PageActions::RestoreBottom();
  }

  last_active_index = stats.active_index;
  last_need_to_arm = stats.need_to_arm;
}
