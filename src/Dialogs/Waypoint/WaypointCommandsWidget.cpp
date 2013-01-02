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

#include "WaypointCommandsWidget.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Language/Language.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/MapTaskManager.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Interface.hpp"
#include "Protection.hpp"
#include "Components.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Pan.hpp"

enum Commands {
  REPLACE_IN_TASK,
  INSERT_IN_TASK,
  APPEND_TO_TASK,
  REMOVE_FROM_TASK,
  SET_HOME,
  PAN,
};

static bool
ReplaceInTask(ProtectedTaskManager &task_manager,
              const Waypoint &waypoint)
{
  switch (MapTaskManager::ReplaceInTask(waypoint)) {
  case MapTaskManager::SUCCESS:
    task_manager.TaskSaveDefault();
    return true;

  case MapTaskManager::NOTASK:
    ShowMessageBox(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;

  case MapTaskManager::UNMODIFIED:
    ShowMessageBox(_("No active task point."), _("Replace in task"),
                MB_OK | MB_ICONINFORMATION);
    break;

  case MapTaskManager::INVALID:
    ShowMessageBox(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;

  case MapTaskManager::MUTATED_TO_GOTO:
  case MapTaskManager::MUTATED_FROM_GOTO:
    break;
  }

  return false;
}

static bool
InsertInTask(ProtectedTaskManager &task_manager,
             const Waypoint &waypoint)
{
  switch (MapTaskManager::InsertInTask(waypoint)) {
  case MapTaskManager::SUCCESS:
    task_manager.TaskSaveDefault();
    return true;

  case MapTaskManager::NOTASK:
    ShowMessageBox(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;

  case MapTaskManager::UNMODIFIED:
  case MapTaskManager::INVALID:
    ShowMessageBox(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;

  case MapTaskManager::MUTATED_TO_GOTO:
    ShowMessageBox(_("Created Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    return true;

  case MapTaskManager::MUTATED_FROM_GOTO:
    ShowMessageBox(_("Created 2-point task from Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    return true;
  }

  return false;
}

static bool
AppendToTask(ProtectedTaskManager &task_manager,
             const Waypoint &waypoint)
{
  switch (MapTaskManager::AppendToTask(waypoint)) {
  case MapTaskManager::SUCCESS:
    task_manager.TaskSaveDefault();
    return true;

  case MapTaskManager::NOTASK:
    ShowMessageBox(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;

  case MapTaskManager::UNMODIFIED:
  case MapTaskManager::INVALID:
    ShowMessageBox(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;

  case MapTaskManager::MUTATED_TO_GOTO:
    ShowMessageBox(_("Created Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    return true;

  case MapTaskManager::MUTATED_FROM_GOTO:
    ShowMessageBox(_("Created 2-point task from Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    return true;
  }

  return false;
}

static bool
RemoveFromTask(ProtectedTaskManager &task_manager,
               const Waypoint &waypoint)
{
  switch (MapTaskManager::RemoveFromTask(waypoint)) {
  case MapTaskManager::SUCCESS:
    task_manager.TaskSaveDefault();
    return true;

  case MapTaskManager::NOTASK:
    ShowMessageBox(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;

  case MapTaskManager::UNMODIFIED:
    ShowMessageBox(_("Waypoint not in task."), _("Remove from task"),
                MB_OK | MB_ICONINFORMATION);
    break;

  case MapTaskManager::INVALID:
    ShowMessageBox(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;

  case MapTaskManager::MUTATED_FROM_GOTO:
  case MapTaskManager::MUTATED_TO_GOTO:
    break;
  }

  return false;
}

static void
SetHome(const Waypoint &waypoint)
{
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  settings_computer.poi.SetHome(waypoint);

  {
    ScopeSuspendAllThreads suspend;
    WaypointGlue::SetHome(way_points, terrain,
                          settings_computer,
                          device_blackboard, false);
    WaypointGlue::SaveHome(settings_computer);
  }
}

static bool
ActivatePan(const Waypoint &waypoint)
{
  return PanTo(waypoint.location);
}

void
WaypointCommandsWidget::OnAction(int id)
{
  switch (id) {
  case REPLACE_IN_TASK:
    if (ReplaceInTask(*task_manager, waypoint) && form != NULL)
      form->SetModalResult(mrOK);
    break;

  case INSERT_IN_TASK:
    if (InsertInTask(*task_manager, waypoint) && form != NULL)
      form->SetModalResult(mrOK);
    break;

  case APPEND_TO_TASK:
    if (AppendToTask(*task_manager, waypoint) && form != NULL)
      form->SetModalResult(mrOK);
    break;

  case REMOVE_FROM_TASK:
    if (RemoveFromTask(*task_manager, waypoint) && form != NULL)
      form->SetModalResult(mrOK);
    break;

  case SET_HOME:
    SetHome(waypoint);
    if (form != NULL)
      form->SetModalResult(mrOK);
    break;

  case PAN:
    if (ActivatePan(waypoint) && form != NULL)
      form->SetModalResult(mrOK);
    break;
  }
}

void
WaypointCommandsWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  if (task_manager != NULL) {
    AddButton(_("Replace In Task"), this, REPLACE_IN_TASK);
    AddButton(_("Insert In Task"), this, INSERT_IN_TASK);
    AddButton(_("Append To Task"), this, APPEND_TO_TASK);

    if (MapTaskManager::GetIndexInTask(waypoint) >= 0)
      AddButton(_("Remove From Task"), this, REMOVE_FROM_TASK);
  }

  AddButton(_("Set As New Home"), this, SET_HOME);
  AddButton(_("Pan To Waypoint"), this, PAN);
}
