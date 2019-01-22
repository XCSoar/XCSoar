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
#include "Event/KeyCode.hpp"
#include "Screen/SingleWindow.hpp"
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

enum Buttons {
  MAP = 100,
};

TaskManagerDialog::~TaskManagerDialog()
{
  delete task;
}

bool
TaskManagerDialog::KeyPress(unsigned key_code)
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
TaskManagerDialog::OnPageFlipped()
{
  RestoreTaskView();
  UpdateCaption();
  TabWidget::OnPageFlipped();
}

void
TaskManagerDialog::OnAction(int id)
{
  switch (id) {
  case MAP:
    TaskViewClicked();
    break;
  }
}

void
TaskManagerDialog::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  task = protected_task_manager->TaskClone();

  /* create the controls */

  SetExtra(new ButtonWidget(new TaskMapButtonRenderer(UIGlobals::GetMapLook()),
                            *this, MAP));

  TabWidget::Initialise(parent, rc);

  /* create pages */

  TaskPropertiesPanel *wProps =
    new TaskPropertiesPanel(*this, &task, &modified);

  TaskClosePanel *wClose = new TaskClosePanel(*this, &modified,
                                              UIGlobals::GetDialogLook());

  const MapLook &look = UIGlobals::GetMapLook();
  Widget *wEdit = CreateTaskEditPanel(*this, look.task, look.airspace,
                                      &task, &modified);

  TaskMiscPanel *list_tab = new TaskMiscPanel(*this, &task, &modified);

  const bool enable_icons =
    CommonInterface::GetUISettings().dialog.tab_style
    == DialogSettings::TabStyle::Icon;
  const IconLook &icons = UIGlobals::GetIconLook();
  const auto *TurnPointIcon = enable_icons ? &icons.hBmpTabTask : nullptr;
  const auto *BrowseIcon = enable_icons ? &icons.hBmpTabWrench : nullptr;
  const auto *PropertiesIcon = enable_icons ? &icons.hBmpTabSettings : nullptr;

  AddTab(wEdit, _("Turn Points"), TurnPointIcon);
  AddTab(list_tab, _("Manage"), BrowseIcon);
  AddTab(wProps, _("Rules"), PropertiesIcon);
  AddTab(wClose, _("Close"));

  UpdateCaption();
}

void
TaskManagerDialog::Show(const PixelRect &rc)
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

  if (!task->TaskSize() || task->CheckTask()) {

    { // this must be done in thread lock because it potentially changes the
      // waypoints database
      ScopeSuspendAllThreads suspend;
      task->CheckDuplicateWaypoints(way_points);
      way_points.Optimise();
    }

    protected_task_manager->TaskCommit(*task);

    try {
      protected_task_manager->TaskSaveDefault();
    } catch (const std::runtime_error &e) {
      ShowError(e, _("Failed to save file."));
      return false;
    }

    modified = false;
    return true;
  }

  ShowMessageBox(getTaskValidationErrors(task->GetFactory().GetValidationErrors()),
    _("Validation Errors"), MB_OK | MB_ICONEXCLAMATION);

  return (ShowMessageBox(_("Task not valid. Changes will be lost.\nContinue?"),
                      _("Task Manager"), MB_YESNO | MB_ICONQUESTION) == IDYES);
}

void
TaskManagerDialog::Revert()
{
  // create new task first to guarantee pointers are different
  OrderedTask *temp = protected_task_manager->TaskClone();
  delete task;
  task = temp;
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
  renderer.SetTask(task);
  modified = false;
}

void
dlgTaskManagerShowModal()
{
  if (protected_task_manager == nullptr)
    return;

  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  TaskManagerDialog tm(dialog);

  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Task Manager"), &tm);
  dialog.ShowModal();
  dialog.StealWidget();
}
