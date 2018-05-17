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

#include "WaypointCommandsWidget.hpp"
#include "WaypointDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"
#include "Form/Form.hpp"
#include "Language/Language.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/MapTaskManager.hpp"
#include "Interface.hpp"
#include "Protection.hpp"
#include "Components.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Pan.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Profile/Current.hpp"

enum Commands {
  REPLACE_IN_TASK,
  INSERT_IN_TASK,
  APPEND_TO_TASK,
  REMOVE_FROM_TASK,
  SET_HOME,
  PAN,
  SET_ACTIVE_FREQUENCY,
  SET_STANDBY_FREQUENCY,
  EDIT,
};

static bool
ReplaceInTask(ProtectedTaskManager &task_manager,
              WaypointPtr waypoint)
{
  switch (MapTaskManager::ReplaceInTask(std::move(waypoint))) {
  case MapTaskManager::SUCCESS:
    try {
      task_manager.TaskSaveDefault();
    } catch (const std::runtime_error &e) {
      ShowError(e, _("Failed to save file."));
      return false;
    }

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
             WaypointPtr waypoint)
{
  switch (MapTaskManager::InsertInTask(std::move(waypoint))) {
  case MapTaskManager::SUCCESS:
    try {
      task_manager.TaskSaveDefault();
    } catch (const std::runtime_error &e) {
      ShowError(e, _("Failed to save file."));
      return false;
    }

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
             WaypointPtr waypoint)
{
  switch (MapTaskManager::AppendToTask(std::move(waypoint))) {
  case MapTaskManager::SUCCESS:
    try {
      task_manager.TaskSaveDefault();
    } catch (const std::runtime_error &e) {
      ShowError(e, _("Failed to save file."));
      return false;
    }

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
    try {
      task_manager.TaskSaveDefault();
    } catch (const std::runtime_error &e) {
      ShowError(e, _("Failed to save file."));
      return false;
    }

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
                          settings_computer.poi, settings_computer.team_code,
                          device_blackboard, false);
    WaypointGlue::SaveHome(Profile::map,
                           settings_computer.poi, settings_computer.team_code);
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
  MessageOperationEnvironment env;

  switch (id) {
  case REPLACE_IN_TASK:
    if (ReplaceInTask(*task_manager, waypoint) && form != nullptr)
      form->SetModalResult(mrOK);
    break;

  case INSERT_IN_TASK:
    if (InsertInTask(*task_manager, waypoint) && form != nullptr)
      form->SetModalResult(mrOK);
    break;

  case APPEND_TO_TASK:
    if (AppendToTask(*task_manager, waypoint) && form != nullptr)
      form->SetModalResult(mrOK);
    break;

  case REMOVE_FROM_TASK:
    if (RemoveFromTask(*task_manager, *waypoint) && form != nullptr)
      form->SetModalResult(mrOK);
    break;

  case SET_HOME:
    SetHome(*waypoint);
    if (form != nullptr)
      form->SetModalResult(mrOK);
    break;

  case PAN:
    if (ActivatePan(*waypoint) && form != nullptr)
      form->SetModalResult(mrOK);
    break;

  case SET_ACTIVE_FREQUENCY:
    device_blackboard->SetActiveFrequency(waypoint->radio_frequency,
                                          waypoint->name.c_str(), env);
    break;

  case SET_STANDBY_FREQUENCY:
    device_blackboard->SetStandbyFrequency(waypoint->radio_frequency,
                                           waypoint->name.c_str(), env);
    break;

  case EDIT:
    {
      Waypoint wp_copy = *waypoint;

      /* move to user.cup */
      wp_copy.origin = WaypointOrigin::USER;

      if (dlgWaypointEditShowModal(wp_copy)) {
        // TODO: refresh data instead of closing dialog?
        form->SetModalResult(mrOK);

        {
          ScopeSuspendAllThreads suspend;
          way_points.Replace(waypoint, std::move(wp_copy));
          way_points.Optimise();
        }

        try {
          WaypointGlue::SaveWaypoints(way_points);
        } catch (const std::runtime_error &e) {
          ShowError(e, _("Failed to save waypoints"));
        }
      }
    }
    break;
  }
}

void
WaypointCommandsWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  if (task_manager != nullptr) {
    AddButton(_("Replace in Task"), *this, REPLACE_IN_TASK);
    AddButton(_("Insert in Task"), *this, INSERT_IN_TASK);
    AddButton(_("Append to Task"), *this, APPEND_TO_TASK);

    if (MapTaskManager::GetIndexInTask(*waypoint) >= 0)
      AddButton(_("Remove from Task"), *this, REMOVE_FROM_TASK);
  }

  AddButton(_("Set as New Home"), *this, SET_HOME);
  AddButton(_("Pan to Waypoint"), *this, PAN);
  AddButton(_("Set Active Frequency"), *this, SET_ACTIVE_FREQUENCY);
  AddButton(_("Set Standby Frequency"), *this, SET_STANDBY_FREQUENCY);

  if (allow_edit)
    AddButton(_("Edit"), *this, EDIT);
}
