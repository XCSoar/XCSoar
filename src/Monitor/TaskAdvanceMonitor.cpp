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

#include "TaskAdvanceMonitor.hpp"
#include "PageActions.hpp"
#include "Widget/QuestionWidget.hpp"
#include "Form/ActionListener.hpp"
#include "Language/Language.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/TaskAdvance.hpp"
#include "Components.hpp"
#include "Interface.hpp"

class TaskAdvanceWidget final
  : public QuestionWidget, private ActionListener {
  TaskAdvanceMonitor &monitor;

  enum Action {
    DISMISS,
    ARM_ADVANCE,
  };

public:
  TaskAdvanceWidget(TaskAdvanceMonitor &_monitor)
    :QuestionWidget(_("In sector, arm advance when ready"), *this),
     monitor(_monitor) {
    AddButton(_("Arm"), ARM_ADVANCE);
    AddButton(_("Dismiss"), DISMISS);
  }

  ~TaskAdvanceWidget() {
    assert(monitor.widget == this);
    monitor.widget = nullptr;
  }

private:
  virtual void OnAction(int id) override;
};

void
TaskAdvanceWidget::OnAction(int id)
{
  switch ((Action)id) {
  case DISMISS:
    break;

  case ARM_ADVANCE: {
    ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
    TaskAdvance &advance = task_manager->SetTaskAdvance();
    advance.SetArmed(true);
  }
  }

  PageActions::RestoreBottom();
}

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
