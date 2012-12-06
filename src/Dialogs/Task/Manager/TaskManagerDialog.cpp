/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "TaskCalculatorPanel.hpp"
#include "TaskEditPanel.hpp"
#include "TaskPropertiesPanel.hpp"
#include "TaskMiscPanel.hpp"
#include "TaskClosePanel.hpp"
#include "../TaskDialogs.hpp"
#include "../dlgTaskHelpers.hpp"
#include "UIGlobals.hpp"
#include "Look/IconLook.hpp"
#include "Dialogs/Message.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/SingleWindow.hpp"
#include "Components.hpp"
#include "Gauge/TaskView.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/TaskStore.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"
#include "Logger/Logger.hpp"
#include "Protection.hpp"
#include "Look/Look.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/TabBar.hpp"
#include "Form/Draw.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

#include <assert.h>
#include <stdio.h>

enum Buttons {
  TARGET,
};

struct TaskManagerLayout {
  PixelRect task_view, target_button, tab_bar;
  bool vertical;
};

/* TODO: eliminate all global variables */
static TaskManagerDialog *instance;
static unsigned TurnpointTab = 0;
static unsigned PropertiesTab = 0;

namespace dlgTaskManager {
  /**
   * paints the task int the frame
   * @param Sender the frame in which to paint the task
   * @param canvas the canvas in which to paint the task
   */
  static void OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas);
}

TaskManagerDialog::~TaskManagerDialog()
{
  delete task_view;
  delete target_button;
  delete tab_bar;
  delete task;
}

void
TaskManagerDialog::OnAction(int id)
{
  switch (id) {
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

  layout.vertical = rc.right > rc.left;
  if (rc.right > rc.left) {
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
  task_view = new WndOwnerDrawFrame(client_area, layout.task_view,
                                    hidden, dlgTaskManager::OnTaskPaint);

  ButtonWindowStyle button_style(hidden);
  button_style.TabStop();
  target_button = new WndButton(client_area, GetLook(), _("Target"),
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
  TaskEditPanel *wEdit = new TaskEditPanel(*this, look.task, look.airspace,
                                           &task, &modified);

  TaskMiscPanel *list_tab = new TaskMiscPanel(*this, &task, &modified);

  const bool enable_icons =
    CommonInterface::GetUISettings().dialog.tab_style
    == DialogSettings::TabStyle::Icon;
  const IconLook &icons = UIGlobals::GetIconLook();
  const Bitmap *CalcIcon = enable_icons ? &icons.hBmpTabCalculator : NULL;
  const Bitmap *TurnPointIcon = enable_icons ? &icons.hBmpTabTask : NULL;
  const Bitmap *BrowseIcon = enable_icons ? &icons.hBmpTabWrench : NULL;
  const Bitmap *PropertiesIcon = enable_icons ? &icons.hBmpTabSettings : NULL;

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
  title.Format(_T("%s - %s"), _("Task Manager"),
               tab_bar->GetButtonCaption(tab_bar->GetCurrentPage()));
  SetCaption(title);
}

void
TaskManagerDialog::InvalidateTaskView()
{
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

static bool
OnTaskViewClick(WndOwnerDrawFrame *Sender, PixelScalar x, PixelScalar y)
{
  instance->TaskViewClicked();
  return true;
}

void
TaskManagerDialog::ShowTaskView(void (*paint)(WndOwnerDrawFrame *sender,
                                              Canvas &canvas))
{
  RestoreTaskView();
  task_view->SetOnPaintNotify(paint);
  task_view->SetOnMouseDownNotify(OnTaskViewClick);
  task_view->Show();
}

void
TaskManagerDialog::ShowTaskView()
{
  ShowTaskView(dlgTaskManager::OnTaskPaint);
}

void
TaskManagerDialog::ResetTaskView()
{
  task_view->Hide();
  RestoreTaskView();
  task_view->SetOnPaintNotify(dlgTaskManager::OnTaskPaint);
  task_view->SetOnMouseDownNotify(OnTaskViewClick);
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

void
dlgTaskManager::OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  const MapLook &look = UIGlobals::GetMapLook();
  const NMEAInfo &basic = CommonInterface::Basic();
  PaintTask(canvas, Sender->GetClientRect(), instance->GetTask(),
            basic.location_available, basic.location,
            XCSoarInterface::GetMapSettings(),
            look.task, look.airspace,
            terrain, &airspace_database,
            true);
}

bool
TaskManagerDialog::Commit()
{
  if (!modified)
    return true;

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
dlgTaskManagerShowModal(SingleWindow &parent)
{
  if (protected_task_manager == NULL)
    return;

  TaskManagerDialog dialog(UIGlobals::GetDialogLook());
  instance = &dialog;

  dialog.Create(parent);

  dialog.ShowModal();
  dialog.Destroy();
}
