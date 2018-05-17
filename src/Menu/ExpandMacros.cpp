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

#include "Menu/ButtonLabel.hpp"
#include "Language/Language.hpp"
#include "Logger/Logger.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Gauge/BigTrafficWidget.hpp"
#include "Computer/Settings.hpp"
#include "Components.hpp"
#include "DataGlobals.hpp"
#include "MapSettings.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Device/device.hpp"
#include "PageActions.hpp"
#include "Util/DollarExpand.hpp"
#include "Util/Macros.hpp"
#include "Net/HTTP/Features.hpp"
#include "UIState.hpp"

#include <stdlib.h>

static const TCHAR *
ExpandTaskMacros(const TCHAR *name,
                 bool &invalid,
                 const DerivedInfo &calculated,
                 const ComputerSettings &settings_computer)
{
  const TaskStats &task_stats = calculated.task_stats;
  const TaskStats &ordered_task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (StringIsEqual(name, _T("CheckTaskResumed"))) {
    // TODO code: check, does this need to be set with temporary task?
    invalid |= common_stats.task_type == TaskType::ABORT ||
      common_stats.task_type == TaskType::GOTO;
    return _T("");
  } else if (StringIsEqual(name, _T("CheckTask"))) {
    invalid |= !task_stats.task_valid;
    return _T("");
  }

  if (protected_task_manager == nullptr) {
    invalid = true;
    return nullptr;
  }

  ProtectedTaskManager::Lease task_manager(*protected_task_manager);

  const AbstractTask *task = task_manager->GetActiveTask();
  if (task == nullptr || !task_stats.task_valid ||
      common_stats.task_type == TaskType::GOTO) {

    if (StringIsEqual(name, _T("WaypointNext")) ||
        StringIsEqual(name, _T("WaypointNextArm"))) {
      invalid = true;
      return _("Next Turnpoint");
    } else if (StringIsEqual(name, _T("WaypointPrevious")) ||
               StringIsEqual(name, _T("WaypointPreviousArm"))) {
      invalid = true;
      return _("Previous Turnpoint");
    }
  } else if (common_stats.task_type == TaskType::ABORT) {
    if (StringIsEqual(name, _T("WaypointNext")) ||
        StringIsEqual(name, _T("WaypointNextArm"))) {
      invalid |= !common_stats.active_has_next;
      return common_stats.next_is_last
        ? _("Furthest Landpoint")
        : _("Next Landpoint");
    } else if (StringIsEqual(name, _T("WaypointPrevious")) ||
               StringIsEqual(name, _T("WaypointPreviousArm"))) {
      invalid |= !common_stats.active_has_previous;

      return common_stats.previous_is_first
        ? _("Closest Landpoint")
        : _("Previous Landpoint");
    }

  } else { // ordered task mode

    const bool next_is_final = common_stats.next_is_last;
    const bool previous_is_start = common_stats.previous_is_first;
    const bool has_optional_starts = ordered_task_stats.has_optional_starts;

    if (StringIsEqual(name, _T("WaypointNext"))) {
      // Waypoint\nNext
      invalid |= !common_stats.active_has_next;
      return next_is_final
        ? _("Finish Turnpoint")
        : _("Next Turnpoint");
    } else if (StringIsEqual(name, _T("WaypointPrevious"))) {
      if (has_optional_starts && !common_stats.active_has_previous) {
        return _("Next Startpoint");
      } else {
        invalid |= !common_stats.active_has_previous;
        return previous_is_start
          ? _("Start Turnpoint")
          : _("Previous Turnpoint");
      }

    } else if (StringIsEqual(name, _T("WaypointNextArm"))) {
      // Waypoint\nNext

      switch (task_manager->GetOrderedTask().GetTaskAdvance().GetState()) {
      case TaskAdvance::MANUAL:
      case TaskAdvance::AUTO:
      case TaskAdvance::START_ARMED:
      case TaskAdvance::TURN_ARMED:
        invalid |= !common_stats.active_has_next;
        return next_is_final
          ? _("Finish Turnpoint")
          : _("Next Turnpoint");

      case TaskAdvance::START_DISARMED:
        return _("Arm start");

      case TaskAdvance::TURN_DISARMED:
        return _("Arm turn");
      }

    } else if (StringIsEqual(name, _T("WaypointPreviousArm"))) {

      switch (task_manager->GetOrderedTask().GetTaskAdvance().GetState()) {
      case TaskAdvance::MANUAL:
      case TaskAdvance::AUTO:
      case TaskAdvance::START_DISARMED:
      case TaskAdvance::TURN_DISARMED:

        if (has_optional_starts && !common_stats.active_has_previous) {
          return _("Next Startpoint");
        } else {
          invalid |= !common_stats.active_has_previous;
          return previous_is_start
            ? _("Start Turnpoint")
            : _("Previous Turnpoint");
        }

      case TaskAdvance::START_ARMED:
        return _("Disarm start");

      case TaskAdvance::TURN_ARMED:
        return _("Disarm turn");
      }
    }
  }

  if (StringIsEqual(name, _T("AdvanceArmed"))) {
    switch (task_manager->GetOrderedTask().GetTaskAdvance().GetState()) {
    case TaskAdvance::MANUAL:
      invalid = true;
      return _("Advance\n(manual)");

    case TaskAdvance::AUTO:
      invalid = true;
      return _("Advance\n(auto)");

    case TaskAdvance::START_ARMED:
      return _("Abort\nStart");

    case TaskAdvance::START_DISARMED:
      return _("Arm\nStart");

    case TaskAdvance::TURN_ARMED:
      return _("Abort\nTurn");

    case TaskAdvance::TURN_DISARMED:
      return _("Arm\nTurn");
    }
  } else if (StringIsEqual(name, _T("CheckAutoMc"))) {
    invalid |= !task_stats.task_valid &&
      settings_computer.task.IsAutoMCFinalGlideEnabled();
    return _T("");
  } else if (StringIsEqual(name, _T("TaskAbortToggleActionName"))) {
    if (common_stats.task_type == TaskType::GOTO)
      return ordered_task_stats.task_valid
        ? _("Resume")
        : _("Abort");
    else
      return common_stats.task_type == TaskType::ABORT
        ? _("Resume")
        : _("Abort");
  } else if (StringIsEqual(name, _T("CheckTaskRestart"))) {
    invalid |= !(common_stats.task_type == TaskType::ORDERED &&
                 task_stats.start.task_started);
    return _T("");
  }

  return nullptr;
}

gcc_pure
static const TCHAR *
ExpandTrafficMacros(const TCHAR *name)
{
  TrafficWidget *widget = (TrafficWidget *)
    CommonInterface::main_window->GetFlavourWidget(_T("Traffic"));
  if (widget == nullptr)
    return nullptr;

  if (StringIsEqual(name, _T("TrafficZoomAutoToggleActionName")))
    return widget->GetAutoZoom() ? _("Manual") : _("Auto");
  else if (StringIsEqual(name, _T("TrafficNorthUpToggleActionName")))
    return widget->GetNorthUp() ? _("Track up") : _("North up");
  else
    return nullptr;
}

static const NMEAInfo &
Basic()
{
  return CommonInterface::Basic();
}

static const DerivedInfo &
Calculated()
{
  return CommonInterface::Calculated();
}

static const ComputerSettings &
GetComputerSettings()
{
  return CommonInterface::GetComputerSettings();
}

static const MapSettings &
GetMapSettings()
{
  return CommonInterface::GetMapSettings();
}

static const UIState &
GetUIState()
{
  return CommonInterface::GetUIState();
}

static const TCHAR *
LookupMacro(const TCHAR *name, bool &invalid)
{
  if (StringIsEqual(name, _T("CheckAirspace"))) {
    invalid |= airspace_database.IsEmpty();
    return nullptr;
  }

  const TCHAR *value = ExpandTaskMacros(name, invalid,
                                        Calculated(), GetComputerSettings());
  if (value != nullptr)
    return value;

  value = ExpandTrafficMacros(name);
  if (value != nullptr)
    return value;

  if (StringIsEqual(name, _T("CheckFLARM"))) {
    invalid |= !Basic().flarm.status.available;
    return nullptr;
  } else if (StringIsEqual(name, _T("CheckWeather"))) {
    const auto rasp = DataGlobals::GetRasp();
    invalid |= rasp == nullptr || rasp->GetItemCount() == 0;
    return nullptr;
  } else if (StringIsEqual(name, _T("CheckCircling"))) {
    invalid |= !Calculated().circling;
    return nullptr;
  } else if (StringIsEqual(name, _T("CheckVega"))) {
    invalid |= devVarioFindVega() == nullptr;
    return nullptr;
  } else if (StringIsEqual(name, _T("CheckReplay"))) {
    invalid |= CommonInterface::MovementDetected();
    return nullptr;
  } else if (StringIsEqual(name, _T("CheckWaypointFile"))) {
    invalid |= way_points.IsEmpty();
    return nullptr;
  } else if (StringIsEqual(name, _T("CheckLogger"))) {
    invalid |= Basic().gps.replay;
    return nullptr;
  } else if (StringIsEqual(name, _T("CheckNet"))) {
#ifndef HAVE_HTTP
    invalid = true;
#endif
    return nullptr;
  } else if (StringIsEqual(name, _T("CheckTerrain"))) {
    invalid |= !Calculated().terrain_valid;
    return nullptr;
  } else if (StringIsEqual(name, _T("LoggerActive"))) {
    return logger != nullptr && logger->IsLoggerActive()
      ? _("Stop")
      : _("Start");
  } else if (StringIsEqual(name, _T("SnailTrailToggleName"))) {
    switch (GetMapSettings().trail.length) {
    case TrailSettings::Length::OFF:
      return _("Long");

    case TrailSettings::Length::LONG:
      return _("Short");

    case TrailSettings::Length::SHORT:
      return _("Full");

    case TrailSettings::Length::FULL:
      return _("Off");
    }

    return nullptr;
  } else if (StringIsEqual(name, _T("AirSpaceToggleName"))) {
    return GetMapSettings().airspace.enable ? _("Off") : _("On");
  } else if (StringIsEqual(name, _T("TerrainTopologyToggleName")) ||
             StringIsEqual(name, _T("TerrainTopographyToggleName"))) {
    char val = 0;
    if (GetMapSettings().topography_enabled)
      val++;
    if (GetMapSettings().terrain.enable)
      val += (char)2;

    switch (val) {
    case 0:
      return _("Topography On");

    case 1:
      return _("Terrain On");

    case 2:
      return _("Terrain + Topography");

    case 3:
      return _("Terrain Off");
    }

    return nullptr;
  } else if (StringIsEqual(name, _T("FullScreenToggleActionName"))) {
    return CommonInterface::main_window->GetFullScreen() ? _("Off") : _("On");
  } else if (StringIsEqual(name, _T("ZoomAutoToggleActionName"))) {
    return GetMapSettings().auto_zoom_enabled ? _("Manual") : _("Auto");
  } else if (StringIsEqual(name, _T("TopologyToggleActionName")) ||
             StringIsEqual(name, _T("TopographyToggleActionName"))) {
    return GetMapSettings().topography_enabled ? _("Off") : _("On");
  } else if (StringIsEqual(name, _T("TerrainToggleActionName"))) {
    return GetMapSettings().terrain.enable ? _("Off") : _("On");
  } else if (StringIsEqual(name, _T("AirspaceToggleActionName"))) {
    return GetMapSettings().airspace.enable ? _("Off") : _("On");
  } else if (StringIsEqual(name, _T("MapLabelsToggleActionName"))) {
    static const TCHAR *const labels[] = {
      N_("All"),
      N_("Task & Landables"),
      N_("Task"),
      N_("None"),
      N_("Task & Airfields"),
    };

    static constexpr unsigned int n = ARRAY_SIZE(labels);
    unsigned int i = (unsigned)GetMapSettings().waypoint.label_selection;
    return gettext(labels[(i + 1) % n]);
  } else if (StringIsEqual(name, _T("MacCreadyToggleActionName"))) {
    return GetComputerSettings().task.auto_mc ? _("Manual") : _("Auto");
  } else if (StringIsEqual(name, _T("AuxInfoToggleActionName"))) {
    return GetUIState().auxiliary_enabled ? _("Off") : _("On");
  } else if (StringIsEqual(name, _T("DispModeClimbShortIndicator"))) {
    return GetUIState().force_display_mode == DisplayMode::CIRCLING
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("DispModeCruiseShortIndicator"))) {
    return GetUIState().force_display_mode == DisplayMode::CRUISE
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("DispModeAutoShortIndicator"))) {
    return GetUIState().force_display_mode == DisplayMode::NONE
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("DispModeFinalShortIndicator"))) {
    return GetUIState().force_display_mode == DisplayMode::FINAL_GLIDE
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("AirspaceModeAllShortIndicator"))) {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::ALLON
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("AirspaceModeClipShortIndicator"))) {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::CLIP
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("AirspaceModeAutoShortIndicator"))) {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::AUTO
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("AirspaceModeBelowShortIndicator"))) {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::ALLBELOW
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("AirspaceModeAllOffIndicator"))) {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::ALLOFF
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("SnailTrailOffShortIndicator"))) {
    return GetMapSettings().trail.length == TrailSettings::Length::OFF
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("SnailTrailShortShortIndicator"))) {
    return GetMapSettings().trail.length == TrailSettings::Length::SHORT
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("SnailTrailLongShortIndicator"))) {
    return GetMapSettings().trail.length == TrailSettings::Length::LONG
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("SnailTrailFullShortIndicator"))) {
    return GetMapSettings().trail.length == TrailSettings::Length::FULL
      ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("AirSpaceOffShortIndicator"))) {
    return !GetMapSettings().airspace.enable ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("AirSpaceOnShortIndicator"))) {
    return GetMapSettings().airspace.enable ? _T("*") : _T("");
  } else if (StringIsEqual(name, _T("FlarmDispToggleActionName"))) {
    return CommonInterface::GetUISettings().traffic.enable_gauge
      ? _("Off") : _("On");
  } else if (StringIsEqual(name, _T("ZoomAutoToggleActionName"))) {
    return GetMapSettings().auto_zoom_enabled ? _("Manual") : _("Auto");
  } else if (StringIsEqual(name, _T("NextPageName"))) {
    static TCHAR label[30]; // TODO: oh no, a static string buffer!
    const PageLayout &page =
      CommonInterface::GetUISettings().pages.pages[PageActions::NextIndex()];
    page.MakeTitle(CommonInterface::GetUISettings().info_boxes, label, true);
    return label;
  } else
    return nullptr;
}

bool
ButtonLabel::ExpandMacros(const TCHAR *In, TCHAR *OutBuffer, size_t Size)
{
  // ToDo, check Buffer Size
  bool invalid = false;

  DollarExpand(In, OutBuffer, Size,
               [&invalid](const TCHAR *name){
                 return LookupMacro(name, invalid);
               });

  return invalid;
}
