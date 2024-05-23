// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Pan.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Profile/Current.hpp"
#include "ActionInterface.hpp"
#include "Widget/RowFormWidget.hpp"

static bool
ReplaceInTask(ProtectedTaskManager &task_manager,
              WaypointPtr waypoint)
{
  switch (MapTaskManager::ReplaceInTask(std::move(waypoint))) {
  case MapTaskManager::SUCCESS:
    try {
      task_manager.TaskSaveDefault();
    } catch (...) {
      ShowError(std::current_exception(), _("Failed to save file."));
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
    } catch (...) {
      ShowError(std::current_exception(), _("Failed to save file."));
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
    } catch (...) {
      ShowError(std::current_exception(), _("Failed to save file."));
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
    } catch (...) {
      ShowError(std::current_exception(), _("Failed to save file."));
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
SetHome(Waypoints *way_points, const Waypoint &waypoint)
{
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  settings_computer.poi.SetHome(waypoint);

  {
    ScopeSuspendAllThreads suspend;
    if (way_points != nullptr)
      WaypointGlue::SetHome(*way_points, data_components->terrain.get(),
                            settings_computer.poi, settings_computer.team_code,
                            backend_components->device_blackboard.get(), false);
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
WaypointCommandsWidget::UpdateButtons()
{
  has_freq = waypoint->radio_frequency.IsDefined();
  SetRowEnabled(REPLACE_IN_TASK, task_manager != nullptr);
  SetRowEnabled(INSERT_IN_TASK, task_manager != nullptr);
  SetRowEnabled(APPEND_TO_TASK, task_manager != nullptr);
  SetRowEnabled(REMOVE_FROM_TASK, task_manager != nullptr && MapTaskManager::GetIndexInTask(*waypoint) >= 0);
  
  SetRowEnabled(SET_ACTIVE_FREQUENCY, has_freq);
  SetRowEnabled(SET_STANDBY_FREQUENCY, has_freq);
  
  SetRowEnabled(EDIT, allow_edit && waypoints != nullptr);
}

void
WaypointCommandsWidget::Prepare(ContainerWindow &parent,
                                const PixelRect &rc) noexcept
{

  RowFormWidget::Prepare(parent, rc);
  
  replace_button = AddButton(_("Replace in Task"), [this](){
    if (ReplaceInTask(*task_manager, waypoint) && form != nullptr)
      form->SetModalResult(mrOK);
  });

  insert_button = AddButton(_("Insert in Task"), [this](){
    if (InsertInTask(*task_manager, waypoint) && form != nullptr)
      form->SetModalResult(mrOK);
  });

  append_button = AddButton(_("Append to Task"), [this](){
    if (AppendToTask(*task_manager, waypoint) && form != nullptr)
      form->SetModalResult(mrOK);
  });
    
  remove_button = AddButton(_("Remove from Task"), [this](){
      if (RemoveFromTask(*task_manager, *waypoint) && form != nullptr)
        form->SetModalResult(mrOK);
    });
  
  home_button = AddButton(_("Set as New Home"), [this](){
    SetHome(waypoints, *waypoint);
    if (form != nullptr)
      form->SetModalResult(mrOK);
  });

  pan_button = AddButton(_("Pan to Waypoint"), [this](){
    if (ActivatePan(*waypoint) && form != nullptr)
      form->SetModalResult(mrOK);
  });
  

  set_active_button = AddButton(_("Set Active Frequency"), [this](){
    ActionInterface::SetActiveFrequency(waypoint->radio_frequency,
                                        waypoint->name.c_str());
  });

  set_standby_button = AddButton(_("Set Standby Frequency"), [this](){
    ActionInterface::SetStandbyFrequency(waypoint->radio_frequency,
                                         waypoint->name.c_str());
  });
  
  edit_button = AddButton(_("Edit"), [this](){
    Waypoint wp_copy = *waypoint;

  /* move to user.cup */
  wp_copy.origin = WaypointOrigin::USER;

  if (dlgWaypointEditShowModal(wp_copy) == WaypointEditResult::MODIFIED) {
    // TODO: refresh data instead of closing dialog?
    form->SetModalResult(mrOK);

    {
      ScopeSuspendAllThreads suspend;
      waypoints->Replace(waypoint, std::move(wp_copy));
      waypoints->Optimise();
    }

    try {
      WaypointGlue::SaveWaypoints(*waypoints);
    } catch (...) {
      ShowError(std::current_exception(), _("Failed to save waypoints"));
    }
    }
  });
  UpdateButtons();
}
