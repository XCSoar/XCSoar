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

#include "InputEvents.hpp"
#include "Util/Macros.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Protection.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Units/Units.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "LocalPath.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/TaskFile.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "OS/Path.hpp"

static void
trigger_redraw()
{
  if (!CommonInterface::Basic().location_available)
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
  TaskAdvance &advance = task_manager->SetTaskAdvance();

  if (StringIsEqual(misc, _T("on"))) {
    advance.SetArmed(true);
  } else if (StringIsEqual(misc, _T("off"))) {
    advance.SetArmed(false);
  } else if (StringIsEqual(misc, _T("toggle"))) {
    advance.ToggleArmed();
  } else if (StringIsEqual(misc, _T("show"))) {
    switch (advance.GetState()) {
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
  dlgTaskManagerShowModal();

  trigger_redraw();
}

void
InputEvents::eventGotoLookup(gcc_unused const TCHAR *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (protected_task_manager == NULL)
    return;

  auto wp = ShowWaypointListDialog(basic.location);
  if (wp != NULL) {
    protected_task_manager->DoGoto(std::move(wp));
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
  auto mc = polar.GetMC();

  TaskBehaviour &task_behaviour = CommonInterface::SetComputerSettings().task;

  if (StringIsEqual(misc, _T("up"))) {
    const auto step = Units::ToSysVSpeed(GetUserVerticalSpeedStep());
    ActionInterface::OffsetManualMacCready(step);
  } else if (StringIsEqual(misc, _T("down"))) {
    const auto step = Units::ToSysVSpeed(GetUserVerticalSpeedStep());
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
    case TaskType::ABORT:
      Message::AddMessage(_("Task aborted"));
      break;
    case TaskType::GOTO:
      Message::AddMessage(_("Go to target"));
      break;
    case TaskType::ORDERED:
      Message::AddMessage(_("Ordered task"));
      break;
    default:
      Message::AddMessage(_("No task"));
    }
  } else {
    // toggle
    switch (task_manager->GetMode()) {
    case TaskType::NONE:
    case TaskType::ORDERED:
      task_manager->Abort();
      break;
    case TaskType::GOTO:
      if (task_manager->CheckOrderedTask()) {
        task_manager->Resume();
      } else {
        task_manager->Abort();
      }
      break;
    case TaskType::ABORT:
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
    OrderedTask *task = TaskFile::GetTask(LocalPath(misc),
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
    protected_task_manager->TaskSave(LocalPath(misc));
  }
}

void
InputEvents::eventTaskTransition(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  if (StringIsEqual(misc, _T("start"))) {
    const StartStats &start_stats =
      CommonInterface::Calculated().ordered_task_stats.start;
    if (!start_stats.task_started)
      return;

    TCHAR TempAll[120];
    _stprintf(TempAll, _T("\r\n%s: %s\r\n%s:%s\r\n%s: %s"),
              _("Altitude"),
              FormatUserAltitude(start_stats.altitude).c_str(),
              _("Speed"),
              FormatUserSpeed(start_stats.ground_speed, true).c_str(),
              _("Time"),
              FormatLocalTimeHHMM((int)start_stats.time,
                                  CommonInterface::GetComputerSettings().utc_offset).c_str());
    Message::AddMessage(_("Task start"), TempAll);
  } else if (StringIsEqual(misc, _T("next"))) {
    Message::AddMessage(_("Next turnpoint"));
  } else if (StringIsEqual(misc, _T("finish"))) {
    Message::AddMessage(_("Task finished"));
  }
}

void
InputEvents::eventResetTask(const TCHAR *misc)
{
  if (protected_task_manager == nullptr)
    return;

  protected_task_manager->ResetTask();
}
