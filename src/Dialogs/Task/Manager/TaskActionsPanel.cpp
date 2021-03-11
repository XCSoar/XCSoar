/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "TaskActionsPanel.hpp"
#include "TaskMiscPanel.hpp"
#include "TaskListPanel.hpp"
#include "Internal.hpp"
#include "../dlgTaskHelpers.hpp"
#include "Dialogs/Message.hpp"
#include "Components.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Simulator.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Device/Declaration.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

TaskActionsPanel::TaskActionsPanel(TaskManagerDialog &_dialog,
                                   TaskMiscPanel &_parent,
                                   std::unique_ptr<OrderedTask> &_active_task,
                                   bool *_task_modified) noexcept
  :RowFormWidget(_dialog.GetLook()),
   dialog(_dialog), parent(_parent),
   active_task(_active_task), task_modified(_task_modified) {}

void
TaskActionsPanel::SaveTask()
{
  AbstractTaskFactory &factory = active_task->GetFactory();
  factory.UpdateStatsGeometry();
  if (factory.CheckAddFinish())
    factory.UpdateGeometry();

  const auto errors = active_task->CheckTask();
  if (!IsError(errors)) {
    if (!OrderedTaskSave(*active_task))
      return;

    *task_modified = true;
    dialog.UpdateCaption();
    DirtyTaskListPanel();
  } else {
    ShowMessageBox(getTaskValidationErrors(errors), _("Task not saved"),
        MB_ICONEXCLAMATION);
  }
}

inline void
TaskActionsPanel::OnBrowseClicked()
{
  parent.SetCurrent(1);
}

inline void
TaskActionsPanel::OnNewTaskClicked()
{
  if ((active_task->TaskSize() < 2) ||
      (ShowMessageBox(_("Create new task?"), _("Task New"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {
    active_task->Clear();
    active_task->SetFactory(CommonInterface::GetComputerSettings().task.task_type_default);
    *task_modified = true;
    dialog.SwitchToPropertiesPanel();
  }
}

inline void
TaskActionsPanel::OnDeclareClicked()
{
  const auto errors = active_task->CheckTask();
  if (IsError(errors)) {
    ShowMessageBox(getTaskValidationErrors(errors), _("Declare task"),
                MB_ICONEXCLAMATION);
    return;
  }

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  Declaration decl(settings.logger, settings.plane, active_task.get());
  ExternalLogger::Declare(decl, way_points.GetHome().get());
}

void
TaskActionsPanel::ReClick() noexcept
{
  dialog.TaskViewClicked();
}

void
TaskActionsPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  AddButton(_("New Task"), [this](){ OnNewTaskClicked(); });
  AddButton(_("Declare"), [this](){ OnDeclareClicked(); });
  AddButton(_("Browse"), [this](){ OnBrowseClicked(); });
  AddButton(_("Save"), [this](){ SaveTask(); });

  if (is_simulator())
    /* cannot communicate with real devices in simulator mode */
    SetRowEnabled(DECLARE, false);
}
