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

#include "TaskActionsPanel.hpp"
#include "TaskMiscPanel.hpp"
#include "TaskListPanel.hpp"
#include "Internal.hpp"
#include "../dlgTaskHelpers.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Form/Draw.hpp"
#include "Form/TabBar.hpp"
#include "Components.hpp"
#include "Logger/ExternalLogger.hpp"
#include "UIGlobals.hpp"
#include "Simulator.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Device/Declaration.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TaskActionsPanel *instance;

void
TaskActionsPanel::SaveTask()
{
  (*active_task)->GetFactory().CheckAddFinish();

  if ((*active_task)->CheckTask()) {
    if (!OrderedTaskSave(**active_task))
      return;

    if (list_panel != NULL)
      list_panel->DirtyList();
  } else {
    ShowMessageBox(getTaskValidationErrors(
        (*active_task)->GetFactory().GetValidationErrors()), _("Task not saved"),
        MB_ICONEXCLAMATION);
  }
}

class WndButton;

void
TaskActionsPanel::OnBrowseClicked()
{
  if (wTaskView != NULL)
    dlgTaskManager::TaskViewRestore(wTaskView);
  parent.SetCurrent(1);
}

static void
OnBrowseClicked()
{
  instance->OnBrowseClicked();
}

void
TaskActionsPanel::OnNewTaskClicked()
{
  if (((*active_task)->TaskSize() < 2) ||
      (ShowMessageBox(_("Create new task?"), _("Task New"),
                   MB_YESNO|MB_ICONQUESTION) == IDYES)) {
    (*active_task)->Clear();
    (*active_task)->SetFactory(XCSoarInterface::GetComputerSettings().task.task_type_default);
    *task_modified = true;
    tab_bar.SetCurrentPage(dlgTaskManager::GetPropertiesTab());
    tab_bar.SetFocus();
  }
}

static void
OnNewTaskClicked()
{
  instance->OnNewTaskClicked();
}

static void
OnSaveClicked()
{
  instance->SaveTask();
}

void
TaskActionsPanel::OnDeclareClicked()
{
  if (!(*active_task)->CheckTask()) {
    const TaskValidationErrorVector errors =
      (*active_task)->GetFactory().GetValidationErrors();
    ShowMessageBox(getTaskValidationErrors(errors), _("Declare task"),
                MB_ICONEXCLAMATION);
    return;
  }

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  Declaration decl(settings.logger, settings.plane, *active_task);
  ExternalLogger::Declare(decl, way_points.GetHome());
}

static void
OnDeclareClicked()
{
  instance->OnDeclareClicked();
}

void
TaskActionsPanel::ReClick()
{
  if (wTaskView != NULL)
    dlgTaskManager::OnTaskViewClick(wTaskView, 0, 0);
}

static constexpr CallBackTableEntry task_list_callbacks[] = {
  DeclareCallBackEntry(OnBrowseClicked),
  DeclareCallBackEntry(OnNewTaskClicked),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(OnDeclareClicked),

  DeclareCallBackEntry(NULL)
};

void
TaskActionsPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(task_list_callbacks, parent, rc,
             _T("IDR_XML_TASKACTIONS"));

  instance = this;

  if (is_simulator())
    /* cannot communicate with real devices in simulator mode */
    form.FindByName(_T("cmdDeclare"))->SetEnabled(false);
}

void
TaskActionsPanel::Show(const PixelRect &rc)
{
  if (wTaskView != NULL)
    wTaskView->Show();

  XMLWidget::Show(rc);
}

void
TaskActionsPanel::Hide()
{
  if (wTaskView != NULL)
    dlgTaskManager::ResetTaskView(wTaskView);

  XMLWidget::Hide();
}
