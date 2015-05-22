/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/SingleWindow.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/TaskStore.hpp"
#include "Task/ValidationErrorStrings.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"
#include "Logger/Logger.hpp"
#include "Protection.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/TabBar.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <assert.h>

enum Buttons {
  MAP = 100,
};

struct TaskManagerLayout {
  PixelRect task_view, tab_bar;
  bool vertical;
};

TaskManagerDialog::~TaskManagerDialog()
{
  /* destroy the TabBar first, to have a well-defined destruction
     order; this is necessary because some pages refer to buttons
     belonging to the dialog */
  if (IsDefined())
    DeleteWindow();

  delete task;
}

bool
TaskManagerDialog::KeyPress(unsigned key_code)
{
  if (WindowWidget::KeyPress(key_code) &&
      tab_bar->InvokeKeyPress(key_code))
    return true;

  switch (key_code) {
  case KEY_ESCAPE:
    if (!modified)
      /* close the dialog immediately if nothing was modified */
      return false;

    if (tab_bar->GetCurrentPage() != 3) {
      /* switch to "close" page instead of closing the dialog */
      tab_bar->SetCurrentPage(3);
      tab_bar->FocusCurrentWidget();
      return true;
    }

    return false;

  default:
    return false;
  }
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

gcc_pure
static TaskManagerLayout
CalculateTaskManagerLayout(PixelRect rc)
{
  TaskManagerLayout layout;

  layout.task_view.left = 0;
  layout.task_view.top = 0;

  layout.vertical = rc.right > rc.bottom;
  if (rc.right > rc.bottom) {
    layout.task_view = { 0, 0, Layout::Scale(80), Layout::Scale(52) };
    layout.tab_bar = { 0, Layout::Scale(52), Layout::Scale(80), rc.bottom };
  } else {
    layout.task_view = { 0, 0, Layout::Scale(60), Layout::Scale(76) };
    layout.tab_bar = { Layout::Scale(60), 0, rc.right, Layout::Scale(76) };
  }

  return layout;
}

void
TaskManagerDialog::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  task = protected_task_manager->TaskClone();

  /* create the controls */

  const TaskManagerLayout layout =
    CalculateTaskManagerLayout(rc);

  task_view_position = layout.task_view;

  WindowStyle hidden;
  hidden.Hide();
  task_view.Create(parent, layout.task_view, hidden,
                   new TaskMapButtonRenderer(UIGlobals::GetMapLook()),
                   *this, MAP);

  WindowStyle tab_style;
  tab_style.ControlParent();
  tab_bar = new TabBarControl(parent, GetLook(), layout.tab_bar,
                              tab_style, layout.vertical);
  tab_bar->SetPageFlippedCallback([this]() { UpdateCaption(); });
  SetWindow(tab_bar);

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
  const Bitmap *TurnPointIcon = enable_icons ? &icons.hBmpTabTask : nullptr;
  const Bitmap *BrowseIcon = enable_icons ? &icons.hBmpTabWrench : nullptr;
  const Bitmap *PropertiesIcon = enable_icons ? &icons.hBmpTabSettings : nullptr;

  tab_bar->AddTab(wEdit, _("Turn Points"), TurnPointIcon);
  tab_bar->AddTab(list_tab, _("Manage"), BrowseIcon);
  tab_bar->AddTab(wProps, _("Rules"), PropertiesIcon);
  tab_bar->AddTab(wClose, _("Close"));

  UpdateCaption();
}

void
TaskManagerDialog::Show(const PixelRect &rc)
{
  const TaskManagerLayout layout = CalculateTaskManagerLayout(rc);

  task_view_position = layout.task_view;

  task_view.Move(layout.task_view);
  tab_bar->UpdateLayout(rc, layout.tab_bar, layout.vertical);
  tab_bar->Show();
}

void
TaskManagerDialog::Hide()
{
  task_view.Hide();
  tab_bar->Hide();
}

void
TaskManagerDialog::Move(const PixelRect &rc)
{
  const TaskManagerLayout layout = CalculateTaskManagerLayout(rc);

  task_view_position = layout.task_view;

  task_view.Move(layout.task_view);
  tab_bar->UpdateLayout(rc, layout.tab_bar, layout.vertical);
}

void
TaskManagerDialog::UpdateCaption()
{
  StaticString<128> title;
  if (task->GetName().empty())
    title.Format(_T("%s: %s"), _("Task Manager"),
                 tab_bar->GetButtonCaption(tab_bar->GetCurrentPage()));
  else
    title.Format(_T("%s: %s - %s"), _("Task Manager"),
                 task->GetName().c_str(),
                 tab_bar->GetButtonCaption(tab_bar->GetCurrentPage()));
  dialog.SetCaption(title);
}

void
TaskManagerDialog::InvalidateTaskView()
{
  UpdateCaption();
  task_view.Invalidate();
}

void
TaskManagerDialog::TaskViewClicked()
{
  fullscreen = !fullscreen;
  task_view.Move(fullscreen
                 ? tab_bar->GetPagerPosition() : task_view_position);
}

void
TaskManagerDialog::RestoreTaskView()
{
  if (fullscreen) {
    fullscreen = false;
    task_view.Move(task_view_position);
  }
}

void
TaskManagerDialog::ShowTaskView(const OrderedTask *_task)
{
  RestoreTaskView();
  auto &renderer = (TaskMapButtonRenderer &)task_view.GetRenderer();
  renderer.SetTask(_task);
  task_view.Show();
  task_view.Invalidate();
}

void
TaskManagerDialog::ShowTaskView()
{
  ShowTaskView(task);
}

void
TaskManagerDialog::ResetTaskView()
{
  task_view.Hide();
  RestoreTaskView();

  auto &renderer = (TaskMapButtonRenderer &)task_view.GetRenderer();
  renderer.SetTask(nullptr);
  task_view.Invalidate();
}

void
TaskManagerDialog::SwitchToEditTab()
{
  tab_bar->SetCurrentPage(TurnpointTab);
  tab_bar->SetFocus();
}

void
TaskManagerDialog::SwitchToPropertiesPanel()
{
  tab_bar->SetCurrentPage(PropertiesTab);
  tab_bar->SetFocus();
}

bool
TaskManagerDialog::Commit()
{
  if (!modified)
    return true;

  task->UpdateStatsGeometry();
  modified |= task->GetFactory().CheckAddFinish();

  if (!task->TaskSize() || task->CheckTask()) {

    { // this must be done in thread lock because it potentially changes the
      // waypoints database
      ScopeSuspendAllThreads suspend;
      task->CheckDuplicateWaypoints(way_points);
      way_points.Optimise();
    }

    protected_task_manager->TaskCommit(*task);
    protected_task_manager->TaskSaveDefault();

    modified = false;
    return true;
  }

  ShowMessageBox(getTaskValidationErrors(task->GetFactory().GetValidationErrors()),
    _("Validation Errors"), MB_ICONEXCLAMATION);

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
