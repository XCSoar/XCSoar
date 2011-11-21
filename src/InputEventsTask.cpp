/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "InputEvents.hpp"
#include "Util/Macros.hpp"
#include "LocalTime.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Protection.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "LocalPath.hpp"
#include "MainWindow.hpp"
#include "Dialogs/Task.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Task/ProtectedTaskManager.hpp"

#include <windef.h> /* for MAX_PATH */

static void
trigger_redraw()
{
  if (!XCSoarInterface::Basic().location_available)
    TriggerGPSUpdate();
  TriggerMapUpdate();
}

// ArmAdvance
// Controls waypoint advance trigger:
//     on: Arms the advance trigger
//    off: Disarms the advance trigger
//   toggle: Toggles between armed and disarmed.
//   show: Shows current armed state
void
InputEvents::eventArmAdvance(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  const TaskAdvance::TaskAdvanceState_t mode =
    task_manager->GetTaskAdvance().get_advance_state();
  
  if (_tcscmp(misc, _T("on")) == 0) {
    task_manager->GetTaskAdvance().set_armed(true);
  } else if (_tcscmp(misc, _T("off")) == 0) {
    task_manager->GetTaskAdvance().set_armed(false);
  } else if (_tcscmp(misc, _T("toggle")) == 0) {
    task_manager->GetTaskAdvance().toggle_armed();
  } else if (_tcscmp(misc, _T("show")) == 0) {
    switch (mode) {
    case TaskAdvance::MANUAL:
      Message::AddMessage(_("Advance manually"));
      break;
    case TaskAdvance::AUTO:
      Message::AddMessage(_("Advance automatically"));
      break;
    case TaskAdvance::START_ARMED:
      Message::AddMessage(_("Ready to start"));
      break;
    case TaskAdvance::START_DISARMED:
      Message::AddMessage(_("Hold start"));
      break;
    case TaskAdvance::TURN_ARMED:
      Message::AddMessage(_("Ready to turn"));
      break;
    case TaskAdvance::TURN_DISARMED:
      Message::AddMessage(_("Hold turn"));
      break;
    }
  }
}

void
InputEvents::eventCalculator(gcc_unused const TCHAR *misc)
{
  dlgTaskManagerShowModal(XCSoarInterface::main_window);
}

void
InputEvents::eventGotoLookup(gcc_unused const TCHAR *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (protected_task_manager == NULL)
    return;

  const Waypoint* wp = dlgWaypointSelect(XCSoarInterface::main_window,
                                         basic.location);
  if (wp != NULL) {
    protected_task_manager->DoGoto(*wp);
    trigger_redraw();
  }
}

// MacCready
// Adjusts MacCready settings
// up, down, auto on, auto off, auto toggle, auto show
void
InputEvents::eventMacCready(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  const GlidePolar &polar =
    CommonInterface::SettingsComputer().glide_polar_task;
  fixed mc = polar.GetMC();

  TaskBehaviour &task_behaviour = CommonInterface::SetSettingsComputer().task;

  if (_tcscmp(misc, _T("up")) == 0) {
    mc = std::min(mc + Units::ToSysVSpeed(fixed_one / fixed(10)), fixed(5));
    ActionInterface::SetMacCready(mc);
  } else if (_tcscmp(misc, _T("down")) == 0) {
    mc = std::max(mc - Units::ToSysVSpeed(fixed_one / fixed(10)), fixed_zero);
    ActionInterface::SetMacCready(mc);
  } else if (_tcscmp(misc, _T("auto toggle")) == 0) {
    task_behaviour.auto_mc = !task_behaviour.auto_mc;
    Profile::Set(szProfileAutoMc, task_behaviour.auto_mc);
  } else if (_tcscmp(misc, _T("auto on")) == 0) {
    task_behaviour.auto_mc = true;
    Profile::Set(szProfileAutoMc, true);
  } else if (_tcscmp(misc, _T("auto off")) == 0) {
    task_behaviour.auto_mc = false;
    Profile::Set(szProfileAutoMc, false);
  } else if (_tcscmp(misc, _T("auto show")) == 0) {
    if (task_behaviour.auto_mc) {
      Message::AddMessage(_("Auto. MacCready on"));
    } else {
      Message::AddMessage(_("Auto. MacCready off"));
    }
  } else if (_tcscmp(misc, _T("show")) == 0) {
    TCHAR Temp[100];
    Units::FormatUserVSpeed(mc,
                            Temp, ARRAY_SIZE(Temp),
                            false);
    Message::AddMessage(_("MacCready "), Temp);
  }
}

// AdjustWaypoint
// Adjusts the active waypoint of the task
//  next: selects the next waypoint, stops at final waypoint
//  previous: selects the previous waypoint, stops at start waypoint
//  nextwrap: selects the next waypoint, wrapping back to start after final
//  previouswrap: selects the previous waypoint, wrapping to final after start
void
InputEvents::eventAdjustWaypoint(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  if (_tcscmp(misc, _T("next")) == 0)
    protected_task_manager->IncrementActiveTaskPoint(1); // next
  else if (_tcscmp(misc, _T("nextwrap")) == 0)
    protected_task_manager->IncrementActiveTaskPoint(1); // next - with wrap
  else if (_tcscmp(misc, _T("previous")) == 0)
    protected_task_manager->IncrementActiveTaskPoint(-1); // previous
  else if (_tcscmp(misc, _T("previouswrap")) == 0)
    protected_task_manager->IncrementActiveTaskPoint(-1); // previous with wrap
  else if (_tcscmp(misc, _T("nextarm")) == 0)
    protected_task_manager->IncrementActiveTaskPointArm(1); // arm sensitive next
  else if (_tcscmp(misc, _T("previousarm")) == 0)
    protected_task_manager->IncrementActiveTaskPointArm(-1); // arm sensitive previous

  trigger_redraw();
}

// AbortTask
// Allows aborting and resuming of tasks
// abort: aborts the task if active
// resume: resumes the task if aborted
// toggle: toggles between abort and resume
// show: displays a status message showing the task abort status
void
InputEvents::eventAbortTask(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);

  if (_tcscmp(misc, _T("abort")) == 0)
    task_manager->Abort();
  else if (_tcscmp(misc, _T("resume")) == 0)
    task_manager->Resume();
  else if (_tcscmp(misc, _T("show")) == 0) {
    switch (task_manager->GetMode()) {
    case TaskManager::MODE_ABORT:
      Message::AddMessage(_("Task aborted"));
      break;
    case TaskManager::MODE_GOTO:
      Message::AddMessage(_("Go to target"));
      break;
    case TaskManager::MODE_ORDERED:
      Message::AddMessage(_("Ordered task"));
      break;
    default:
      Message::AddMessage(_("No task"));
    }
  } else {
    // toggle
    switch (task_manager->GetMode()) {
    case TaskManager::MODE_NULL:
    case TaskManager::MODE_ORDERED:
      task_manager->Abort();
      break;
    case TaskManager::MODE_GOTO:
      if (task_manager->CheckOrderedTask()) {
        task_manager->Resume();
      } else {
        task_manager->Abort();
      }
      break;
    case TaskManager::MODE_ABORT:
      task_manager->Resume();
      break;
    default:
      break;
    }
  }

  trigger_redraw();
}

// TaskLoad
// Loads the task of the specified filename
void
InputEvents::eventTaskLoad(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  if (!string_is_empty(misc)) {
    TCHAR buffer[MAX_PATH];
    LocalPath(buffer, misc);

    OrderedTask *task = protected_task_manager->TaskCreate(buffer, &way_points);
    if (task) {
      {
        ScopeSuspendAllThreads suspend;
        task->CheckDuplicateWaypoints(way_points);
        way_points.optimise();
      }
      protected_task_manager->TaskCommit(*task);
      delete task;
    }
  }

  trigger_redraw();
}

// TaskSave
// Saves the task to the specified filename
void
InputEvents::eventTaskSave(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  if (!string_is_empty(misc)) {
    TCHAR buffer[MAX_PATH];
    LocalPath(buffer, misc);
    protected_task_manager->TaskSave(buffer);
  }
}

void
InputEvents::eventTaskTransition(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  if (_tcscmp(misc, _T("start")) == 0) {
    AircraftState start_state = protected_task_manager->GetStartState();

    TCHAR TempTime[40];
    TCHAR TempAlt[40];
    TCHAR TempSpeed[40];
    
    Units::TimeToTextHHMMSigned(TempTime, (int)TimeLocal((int)start_state.time));
    Units::FormatUserAltitude(start_state.altitude,
                              TempAlt, sizeof(TempAlt)/sizeof(TCHAR), true);
    Units::FormatUserSpeed(start_state.ground_speed,
                           TempSpeed, sizeof(TempSpeed)/sizeof(TCHAR), true);
    
    TCHAR TempAll[120];
    _stprintf(TempAll, _T("\r\nAltitude: %s\r\nSpeed:%s\r\nTime: %s"),
              TempAlt, TempSpeed, TempTime);
    Message::AddMessage(_("Task start"), TempAll);
  } else if (_tcscmp(misc, _T("tp")) == 0) {
    Message::AddMessage(_("Next turnpoint"));
  } else if (_tcscmp(misc, _T("finish")) == 0) {
    Message::AddMessage(_("Task finished"));
  } else if (_tcscmp(misc, _T("ready")) == 0) {
    Message::AddMessage(_("In sector, arm advance when ready"));
  }
}
