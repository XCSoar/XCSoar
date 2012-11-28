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

#include "InputEvents.hpp"
#include "Util/Macros.hpp"
#include "LocalTime.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Protection.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Units/Units.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "LocalPath.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/Task.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/TaskFile.hpp"

#include <windef.h> /* for MAX_PATH */

static void
trigger_redraw()
{
  if (!XCSoarInterface::Basic().location_available)
    ForceCalculation();
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

  if (StringIsEqual(misc, _T("on"))) {
    task_manager->GetTaskAdvance().SetArmed(true);
  } else if (StringIsEqual(misc, _T("off"))) {
    task_manager->GetTaskAdvance().SetArmed(false);
  } else if (StringIsEqual(misc, _T("toggle"))) {
    task_manager->GetTaskAdvance().ToggleArmed();
  } else if (StringIsEqual(misc, _T("show"))) {
    switch (task_manager->GetTaskAdvance().GetState()) {
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

  /* quickly propagate the updated values from the TaskManager to the
     InterfaceBlackboard, so they are available immediately */
  task_manager->UpdateCommonStatsTask();
  CommonInterface::ReadCommonStats(task_manager->GetCommonStats());
}

void
InputEvents::eventCalculator(gcc_unused const TCHAR *misc)
{
  dlgTaskManagerShowModal(UIGlobals::GetMainWindow());

  trigger_redraw();
}

void
InputEvents::eventGotoLookup(gcc_unused const TCHAR *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (protected_task_manager == NULL)
    return;

  const Waypoint* wp = ShowWaypointListDialog(UIGlobals::GetMainWindow(),
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
    CommonInterface::GetComputerSettings().polar.glide_polar_task;
  fixed mc = polar.GetMC();

  TaskBehaviour &task_behaviour = CommonInterface::SetComputerSettings().task;

  if (StringIsEqual(misc, _T("up"))) {
    const fixed step = Units::ToSysVSpeed(GetUserVerticalSpeedStep());
    ActionInterface::OffsetManualMacCready(step);
  } else if (StringIsEqual(misc, _T("down"))) {
    const fixed step = Units::ToSysVSpeed(GetUserVerticalSpeedStep());
    ActionInterface::OffsetManualMacCready(-step);
  } else if (StringIsEqual(misc, _T("auto toggle"))) {
    task_behaviour.auto_mc = !task_behaviour.auto_mc;
    Profile::Set(ProfileKeys::AutoMc, task_behaviour.auto_mc);
  } else if (StringIsEqual(misc, _T("auto on"))) {
    task_behaviour.auto_mc = true;
    Profile::Set(ProfileKeys::AutoMc, true);
  } else if (StringIsEqual(misc, _T("auto off"))) {
    task_behaviour.auto_mc = false;
    Profile::Set(ProfileKeys::AutoMc, false);
  } else if (StringIsEqual(misc, _T("auto show"))) {
    if (task_behaviour.auto_mc) {
      Message::AddMessage(_("Auto. MacCready on"));
    } else {
      Message::AddMessage(_("Auto. MacCready off"));
    }
  } else if (StringIsEqual(misc, _T("show"))) {
    TCHAR Temp[100];
    FormatUserVerticalSpeed(mc, Temp, false);
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

  if (StringIsEqual(misc, _T("next")))
    protected_task_manager->IncrementActiveTaskPoint(1); // next
  else if (StringIsEqual(misc, _T("nextwrap")))
    protected_task_manager->IncrementActiveTaskPoint(1); // next - with wrap
  else if (StringIsEqual(misc, _T("previous")))
    protected_task_manager->IncrementActiveTaskPoint(-1); // previous
  else if (StringIsEqual(misc, _T("previouswrap")))
    protected_task_manager->IncrementActiveTaskPoint(-1); // previous with wrap
  else if (StringIsEqual(misc, _T("nextarm")))
    protected_task_manager->IncrementActiveTaskPointArm(1); // arm sensitive next
  else if (StringIsEqual(misc, _T("previousarm")))
    protected_task_manager->IncrementActiveTaskPointArm(-1); // arm sensitive previous

  {
    /* quickly propagate the updated values from the TaskManager to
       the InterfaceBlackboard, so they are available immediately */
    ProtectedTaskManager::ExclusiveLease tm(*protected_task_manager);
    tm->UpdateCommonStatsTask();
    CommonInterface::ReadCommonStats(tm->GetCommonStats());
  }

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

  if (StringIsEqual(misc, _T("abort")))
    task_manager->Abort();
  else if (StringIsEqual(misc, _T("resume")))
    task_manager->Resume();
  else if (StringIsEqual(misc, _T("show"))) {
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
    }
  }

  /* quickly propagate the updated values from the TaskManager to the
     InterfaceBlackboard, so they are available immediately */
  task_manager->UpdateCommonStatsTask();
  CommonInterface::ReadCommonStats(task_manager->GetCommonStats());

  trigger_redraw();
}

// TaskLoad
// Loads the task of the specified filename
void
InputEvents::eventTaskLoad(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  if (!StringIsEmpty(misc)) {
    TCHAR buffer[MAX_PATH];
    LocalPath(buffer, misc);

    OrderedTask *task = TaskFile::GetTask(buffer,
                                          CommonInterface::GetComputerSettings().task,
                                          &way_points, 0);
    if (task) {
      {
        ScopeSuspendAllThreads suspend;
        task->CheckDuplicateWaypoints(way_points);
        way_points.Optimise();
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

  if (!StringIsEmpty(misc)) {
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

  if (StringIsEqual(misc, _T("start"))) {
    AircraftState start_state = protected_task_manager->GetStartState();

    TCHAR TempTime[40];
    TCHAR TempAlt[40];
    TCHAR TempSpeed[40];
    
    FormatSignedTimeHHMM(TempTime, (int)TimeLocal((int)start_state.time));
    FormatUserAltitude(start_state.altitude, TempAlt, true);
    FormatUserSpeed(start_state.ground_speed,TempSpeed, true);
    
    TCHAR TempAll[120];
    _stprintf(TempAll, _T("\r\nAltitude: %s\r\nSpeed:%s\r\nTime: %s"),
              TempAlt, TempSpeed, TempTime);
    Message::AddMessage(_("Task start"), TempAll);
  } else if (StringIsEqual(misc, _T("tp"))) {
    Message::AddMessage(_("Next turnpoint"));
  } else if (StringIsEqual(misc, _T("finish"))) {
    Message::AddMessage(_("Task finished"));
  } else if (StringIsEqual(misc, _T("ready"))) {
    Message::AddMessage(_("In sector, arm advance when ready"));
  }
}
