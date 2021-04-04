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

#include "Internal.hpp"
#include "TaskMapButtonRenderer.hpp"
#include "TaskEditPanel.hpp"
#include "TaskPropertiesPanel.hpp"
#include "TaskMiscPanel.hpp"
#include "TaskClosePanel.hpp"
#include "../TaskDialogs.hpp"
#include "UIGlobals.hpp"
#include "Look/IconLook.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/window/SingleWindow.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Protection.hpp"
#include "Widget/ButtonWidget.hpp"
#include "Widget/TabWidget.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

TaskManagerDialog::TaskManagerDialog(WndForm &_dialog) noexcept
    :TabWidget(Orientation::AUTO),
     dialog(_dialog) {}

TaskManagerDialog::~TaskManagerDialog() noexcept = default;

bool
TaskManagerDialog::KeyPress(unsigned key_code) noexcept
{
  if (TabWidget::KeyPress(key_code))
    return true;

  switch (key_code) {
  case KEY_ESCAPE:
    if (!modified)
      /* close the dialog immediately if nothing was modified */
      return false;

    if (GetCurrentIndex() != 3) {
      /* switch to "close" page instead of closing the dialog */
      SetCurrent(3);
      SetFocus();
      return true;
    }

    return false;

  default:
    return false;
  }
}

void
TaskManagerDialog::OnPageFlipped() noexcept
{
  RestoreTaskView();
  UpdateCaption();
  TabWidget::OnPageFlipped();
}

void
TaskManagerDialog::Initialise(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  task = protected_task_manager->TaskClone();

  /* create the controls */

  SetExtra(std::make_unique<ButtonWidget>(std::make_unique<TaskMapButtonRenderer>(UIGlobals::GetMapLook()),
                                          [this](){ TaskViewClicked(); }));

  TabWidget::Initialise(parent, rc);

  /* create pages */

  const MapLook &look = UIGlobals::GetMapLook();

  const bool enable_icons =
    CommonInterface::GetUISettings().dialog.tab_style
    == DialogSettings::TabStyle::Icon;
  const IconLook &icons = UIGlobals::GetIconLook();
  const auto *TurnPointIcon = enable_icons ? &icons.hBmpTabTask : nullptr;
  const auto *BrowseIcon = enable_icons ? &icons.hBmpTabWrench : nullptr;
  const auto *PropertiesIcon = enable_icons ? &icons.hBmpTabSettings : nullptr;

  AddTab(CreateTaskEditPanel(*this, look.task, look.airspace,
                             task, &modified),
         _("Turn Points"), TurnPointIcon);
  AddTab(std::make_unique<TaskMiscPanel>(*this, task, &modified),
         _("Manage"), BrowseIcon);
  AddTab(std::make_unique<TaskPropertiesPanel>(*this, task, &modified),
         _("Rules"), PropertiesIcon);
  AddTab(std::make_unique<TaskClosePanel>(*this, &modified,
                                          UIGlobals::GetDialogLook()),
         _("Close"));

  UpdateCaption();
}

void
TaskManagerDialog::Show(const PixelRect &rc) noexcept
{
  ResetTaskView();
  TabWidget::Show(rc);
}

void
TaskManagerDialog::UpdateCaption()
{
  StaticString<128> title;
  if (task->GetName().empty())
    title.Format(_T("%s: %s"), _("Task Manager"),
                 GetButtonCaption(GetCurrentIndex()));
  else
    title.Format(_T("%s: %s - %s"), _("Task Manager"),
                 task->GetName().c_str(),
                 GetButtonCaption(GetCurrentIndex()));
  dialog.SetCaption(title);
}

void
TaskManagerDialog::InvalidateTaskView()
{
  UpdateCaption();

  auto &task_view = (ButtonWidget &)GetExtra();
  auto &renderer = (TaskMapButtonRenderer &)task_view.GetRenderer();
  renderer.InvalidateBuffer();
  task_view.Invalidate();
}

void
TaskManagerDialog::ShowTaskView(const OrderedTask *_task)
{
  auto &task_view = (ButtonWidget &)GetExtra();
  auto &renderer = (TaskMapButtonRenderer &)task_view.GetRenderer();
  renderer.SetTask(_task);
  task_view.Invalidate();
}

void
TaskManagerDialog::SwitchToEditTab()
{
  SetCurrent(TurnpointTab);
  SetFocus();
}

void
TaskManagerDialog::SwitchToPropertiesPanel()
{
  SetCurrent(PropertiesTab);
  SetFocus();
}

bool
TaskManagerDialog::Commit()
{
  if (!modified)
    return true;

  modified |= task->GetFactory().CheckAddFinish();
  task->UpdateStatsGeometry();

  const auto errors = task->CheckTask();
  if (!task->TaskSize() || !IsError(errors)) {

    { // this must be done in thread lock because it potentially changes the
      // waypoints database
      ScopeSuspendAllThreads suspend;
      task->CheckDuplicateWaypoints(way_points);
      way_points.Optimise();
    }

    protected_task_manager->TaskCommit(*task);

    try {
      protected_task_manager->TaskSaveDefault();
    } catch (...) {
      ShowError(std::current_exception(), _("Failed to save file."));
      return false;
    }

    modified = false;
    return true;
  }

  ShowMessageBox(getTaskValidationErrors(errors),
    _("Validation Errors"), MB_OK | MB_ICONEXCLAMATION);

  return (ShowMessageBox(_("Task not valid. Changes will be lost.\nContinue?"),
                      _("Task Manager"), MB_YESNO | MB_ICONQUESTION) == IDYES);
}

void
TaskManagerDialog::Revert()
{
  // create new task first to guarantee pointers are different
  task = protected_task_manager->TaskClone();
  /**
   * \todo Having local pointers scattered about is an accident waiting to
   *       happen. Need a semantic that provides the authoritative pointer to
   *       the current task to all.
   */
  /**
   * Update our widget's ordered task because it is now out of date.
   */
  auto &task_view = (ButtonWidget &)GetExtra();
  auto &renderer = (TaskMapButtonRenderer &)task_view.GetRenderer();
  renderer.SetTask(task.get());
  modified = false;
}

void
dlgTaskManagerShowModal()
{
  if (protected_task_manager == nullptr)
    return;

  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<TaskManagerDialog>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, _("Task Manager"));
  dialog.SetWidget(dialog);
  dialog.ShowModal();
}
