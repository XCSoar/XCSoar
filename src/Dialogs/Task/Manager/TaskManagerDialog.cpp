/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "TaskMapWindow.hpp"
#include "TaskCalculatorPanel.hpp"
#include "TaskEditPanel.hpp"
#include "TaskPropertiesPanel.hpp"
#include "TaskMiscPanel.hpp"
#include "TaskClosePanel.hpp"
#include "../TaskDialogs.hpp"
#include "UIGlobals.hpp"
#include "Look/IconLook.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
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
  TARGET,
};

struct TaskManagerLayout {
  PixelRect task_view, target_button, tab_bar;
  bool vertical;
};

TaskManagerDialog::~TaskManagerDialog()
{
  delete task_view;
  delete target_button;
  delete tab_bar;
  delete task;
}

void
TaskManagerDialog::ReinitialiseLayout(const PixelRect &parent_rc)
{
  Move(parent_rc);
}

bool
TaskManagerDialog::OnAnyKeyDown(unsigned key_code)
{
  if (WndForm::OnAnyKeyDown(key_code) ||
      tab_bar->InvokeKeyPress(key_code))
    return true;

  switch (key_code) {
  case KEY_ESCAPE:
    if (!modified)
      /* close the dialog immediately if nothing was modified */
      return false;

    if (tab_bar->GetCurrentPage() != 4) {
      /* switch to "close" page instead of closing the dialog */
      tab_bar->SetCurrentPage(4);
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

  case TARGET:
    dlgTargetShowModal();
    break;

  default:
    WndForm::OnAction(id);
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
    layout.target_button = { Layout::Scale(5), Layout::Scale(18),
                             Layout::Scale(58), Layout::Scale(51) };
    layout.tab_bar = { 0, Layout::Scale(52), Layout::Scale(80), rc.bottom };
  } else {
    layout.task_view = { 0, 0, Layout::Scale(60), Layout::Scale(76) };
    layout.target_button = { Layout::Scale(2), Layout::Scale(2),
                             Layout::Scale(59), Layout::Scale(69) };
    layout.tab_bar = { Layout::Scale(60), 0, rc.right, Layout::Scale(76) };
  }

  return layout;
}

void
TaskManagerDialog::Create(SingleWindow &parent)
{
  WndForm::Create(parent, parent.GetClientRect(), _("Task Manager"));

  task = protected_task_manager->TaskClone();

  /* create the controls */

  ContainerWindow &client_area = GetClientAreaWindow();
  const TaskManagerLayout layout =
    CalculateTaskManagerLayout(client_area.GetClientRect());

  task_view_position = layout.task_view;

  WindowStyle hidden;
  hidden.Hide();
  task_view = new TaskMapWindow(UIGlobals::GetMapLook(), *this, MAP);
  task_view->Create(client_area, layout.task_view, hidden);

  ButtonWindowStyle button_style(hidden);
  button_style.TabStop();
  target_button = new WndButton(client_area, GetLook().button, _("Target"),
                                layout.target_button, button_style,
                                *this, TARGET);

  WindowStyle tab_style;
  tab_style.ControlParent();
  tab_bar = new TabBarControl(client_area, GetLook(), layout.tab_bar,
                              tab_style, layout.vertical);
  tab_bar->SetPageFlippedCallback([this]() { UpdateCaption(); });

  /* create pages */

  TaskPropertiesPanel *wProps =
    new TaskPropertiesPanel(*this, &task, &modified);

  TaskClosePanel *wClose = new TaskClosePanel(*this, &modified);

  TaskCalculatorPanel *wCalculator =
    new TaskCalculatorPanel(UIGlobals::GetDialogLook(), &modified);
  wCalculator->SetTargetButton(target_button);

  const MapLook &look = UIGlobals::GetMapLook();
  Widget *wEdit = CreateTaskEditPanel(*this, look.task, look.airspace,
                                      &task, &modified);

  TaskMiscPanel *list_tab = new TaskMiscPanel(*this, &task, &modified);

  const bool enable_icons =
    CommonInterface::GetUISettings().dialog.tab_style
    == DialogSettings::TabStyle::Icon;
  const IconLook &icons = UIGlobals::GetIconLook();
  const Bitmap *CalcIcon = enable_icons ? &icons.hBmpTabCalculator : nullptr;
  const Bitmap *TurnPointIcon = enable_icons ? &icons.hBmpTabTask : nullptr;
  const Bitmap *BrowseIcon = enable_icons ? &icons.hBmpTabWrench : nullptr;
  const Bitmap *PropertiesIcon = enable_icons ? &icons.hBmpTabSettings : nullptr;

  tab_bar->AddTab(wCalculator, _("Calculator"), CalcIcon);

  if (layout.vertical) {
    tab_bar->AddTab(wEdit, _("Turn Points"), TurnPointIcon);
    TurnpointTab = 1;

    tab_bar->AddTab(list_tab, _("Manage"), BrowseIcon);

    tab_bar->AddTab(wProps, _("Rules"), PropertiesIcon);
    PropertiesTab = 3;

    tab_bar->AddTab(wClose, _("Close"));

    tab_bar->SetCurrentPage(0);
  } else {
    tab_bar->AddTab(wClose, _("Close"));

    tab_bar->AddTab(wEdit, _("Turn Points"), TurnPointIcon);
    TurnpointTab = 2;

    tab_bar->AddTab(list_tab, _("Manage"), BrowseIcon);

    tab_bar->AddTab(wProps, _("Rules"), PropertiesIcon);
    PropertiesTab = 4;

    tab_bar->SetCurrentPage(0);
  }

  UpdateCaption();
}

void
TaskManagerDialog::OnResize(PixelSize new_size)
{
  WndForm::OnResize(new_size);

  ContainerWindow &client_area = GetClientAreaWindow();
  const PixelRect rc = client_area.GetClientRect();
  const TaskManagerLayout layout = CalculateTaskManagerLayout(rc);

  task_view_position = layout.task_view;

  if (task_view != nullptr)
    task_view->Move(layout.task_view);

  if (target_button != nullptr)
    target_button->Move(layout.target_button);

  if (tab_bar != nullptr)
    tab_bar->UpdateLayout(rc, layout.tab_bar, layout.vertical);
}

void
TaskManagerDialog::Destroy()
{
  /* destroy the TabBar first, to have a well-defined destruction
     order; this is necessary because some pages refer to buttons
     belonging to the dialog */
  tab_bar->Destroy();

  WndForm::Destroy();
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
  SetCaption(title);
}

void
TaskManagerDialog::InvalidateTaskView()
{
  UpdateCaption();
  task_view->Invalidate();
}

void
TaskManagerDialog::TaskViewClicked()
{
  fullscreen = !fullscreen;
  task_view->Move(fullscreen
                  ? tab_bar->GetPagerPosition() : task_view_position);
}

void
TaskManagerDialog::RestoreTaskView()
{
  if (fullscreen) {
    fullscreen = false;
    task_view->Move(task_view_position);
  }
}

void
TaskManagerDialog::ShowTaskView(const OrderedTask *_task)
{
  RestoreTaskView();
  task_view->SetTask(_task);
  task_view->Show();
}

void
TaskManagerDialog::ShowTaskView()
{
  ShowTaskView(task);
}

void
TaskManagerDialog::ResetTaskView()
{
  task_view->Hide();
  RestoreTaskView();
  task_view->SetTask(nullptr);
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

  TaskManagerDialog dialog(UIGlobals::GetDialogLook());

  dialog.Create(UIGlobals::GetMainWindow());

  dialog.ShowModal();
  dialog.Destroy();
}
