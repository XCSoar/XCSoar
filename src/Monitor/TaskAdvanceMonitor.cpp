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

/**
 * The question while the glider is in the start area or in an AAT
 * area, or after it has left an AAT area it has reached (the start
 * does not ask from outside).  Only these task points are armed, and
 * the start is always task point 0.
 */
[[gnu::pure]]
static const char *
GetArmMessage(const TaskStats &stats) noexcept
{
  if (stats.active_index == 0)
    return _("In start area, arm start when ready");

  return stats.inside_oz
    ? _("In area, arm turn when ready")
    : _("Area reached, arm turn when ready");
}

class TaskAdvanceWidget final
  : public QuestionWidget
{
  TaskAdvanceMonitor &monitor;

  /** the message shown, one of the GetArmMessage() strings */
  const char *message;

public:
  TaskAdvanceWidget(TaskAdvanceMonitor &_monitor, const char *_message)
    :QuestionWidget(_message),
     monitor(_monitor), message(_message) {
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

  /**
   * Show this message if it differs from the current one; setting the
   * text repaints the bottom area, which is costly on e-paper.
   */
  void Update(const char *_message) noexcept {
    if (_message != message)
      SetMessage(message = _message);
  }
};

void
TaskAdvanceMonitor::Check()
{
  const TaskStats &stats = CommonInterface::Calculated().ordered_task_stats;

  if (stats.need_to_arm) {
    if (widget == nullptr && (!last_need_to_arm ||
                              stats.active_index != last_active_index)) {
      widget = new TaskAdvanceWidget(*this, GetArmMessage(stats));
      PageActions::SetCustomBottom(widget);
    } else if (widget != nullptr)
      widget->Update(GetArmMessage(stats));
  } else {
    if (widget != nullptr)
      PageActions::RestoreBottom();
  }

  last_active_index = stats.active_index;
  last_need_to_arm = stats.need_to_arm;
}
