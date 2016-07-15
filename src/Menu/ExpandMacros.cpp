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
#include "Util/TruncateString.hpp"
#include "Util/Macros.hpp"
#include "Net/HTTP/Features.hpp"
#include "UIState.hpp"

#include <stdlib.h>

#include <windef.h> /* for MAX_PATH */

/**
 * Replaces ToReplace with ReplaceWith in String
 * @param String Buffer string
 * @param ToReplace The string that will be replaced
 * @param ReplaceWith The replacement
 * @param Size (?)
 */
static void
ReplaceInString(TCHAR *String, const TCHAR *ToReplace,
                const TCHAR *ReplaceWith, size_t Size)
{
  TCHAR TmpBuf[MAX_PATH];
  size_t iR = _tcslen(ToReplace);
  TCHAR *pC;

  while ((pC = _tcsstr(String, ToReplace)) != nullptr) {
    _tcscpy(TmpBuf, pC + iR);
    _tcscpy(pC, ReplaceWith);
    _tcscat(pC, TmpBuf);
  }
}

/**
 * If Condition is true, Macro in Buffer will be replaced by TrueText,
 * otherwise by FalseText.
 * @param Condition Condition to be checked
 * @param Buffer Buffer string
 * @param Macro The string that will be replaced
 * @param TrueText The replacement if Condition is true
 * @param FalseText The replacement if Condition is false
 * @param Size (?)
 */
static void
CondReplaceInString(bool Condition, TCHAR *Buffer, const TCHAR *Macro,
                    const TCHAR *TrueText, const TCHAR *FalseText, size_t Size)
{
  if (Condition)
    ReplaceInString(Buffer, Macro, TrueText, Size);
  else
    ReplaceInString(Buffer, Macro, FalseText, Size);
}

static bool
ExpandTaskMacros(TCHAR *OutBuffer, size_t Size,
                 const DerivedInfo &calculated,
                 const ComputerSettings &settings_computer)
{
  const TaskStats &task_stats = calculated.task_stats;
  const TaskStats &ordered_task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  bool invalid = false;

  if (_tcsstr(OutBuffer, _T("$(CheckTaskResumed)"))) {
    // TODO code: check, does this need to be set with temporary task?
    if (common_stats.task_type == TaskType::ABORT ||
        common_stats.task_type == TaskType::GOTO)
      invalid = true;
    ReplaceInString(OutBuffer, _T("$(CheckTaskResumed)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckTask)"))) {
    if (!task_stats.task_valid)
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckTask)"), _T(""), Size);
  }

  if (protected_task_manager == nullptr)
    return true;

  ProtectedTaskManager::Lease task_manager(*protected_task_manager);

  const AbstractTask *task = task_manager->GetActiveTask();
  if (task == nullptr || !task_stats.task_valid ||
      common_stats.task_type == TaskType::GOTO) {

    if (_tcsstr(OutBuffer, _T("$(WaypointNext)"))) {
      invalid = true;
      ReplaceInString(OutBuffer, _T("$(WaypointNext)"),
          _("Next Turnpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointPrevious)"))) {
      invalid = true;
      ReplaceInString(OutBuffer, _T("$(WaypointPrevious)"),
          _("Previous Turnpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointNextArm)"))) {
      invalid = true;
      ReplaceInString(OutBuffer, _T("$(WaypointNextArm)"),
          _("Next Turnpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointPreviousArm)"))) {
      invalid = true;
      ReplaceInString(OutBuffer, _T("$(WaypointPreviousArm)"),
          _("Previous Turnpoint"), Size);
    }

  } else if (common_stats.task_type == TaskType::ABORT) {

    if (_tcsstr(OutBuffer, _T("$(WaypointNext)"))) {
      if (!common_stats.active_has_next)
        invalid = true;

      CondReplaceInString(common_stats.next_is_last,
                          OutBuffer,
                          _T("$(WaypointNext)"),
                          _("Furthest Landpoint"),
                          _("Next Landpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointPrevious)"))) {
      if (!common_stats.active_has_previous)
        invalid = true;

      CondReplaceInString(common_stats.previous_is_first,
                          OutBuffer,
                          _T("$(WaypointPrevious)"),
                          _("Closest Landpoint"),
                          _("Previous Landpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointNextArm)"))) {
      if (!common_stats.active_has_next)
        invalid = true;

      CondReplaceInString(common_stats.next_is_last,
                          OutBuffer,
                          _T("$(WaypointNextArm)"),
                          _("Furthest Landpoint"),
                          _("Next Landpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointPreviousArm)"))) {
      if (!common_stats.active_has_previous)
        invalid = true;

      CondReplaceInString(common_stats.previous_is_first,
                          OutBuffer,
                          _T("$(WaypointPreviousArm)"),
                          _("Closest Landpoint"),
                          _("Previous Landpoint"), Size);
    }

  } else { // ordered task mode

    const bool next_is_final = common_stats.next_is_last;
    const bool previous_is_start = common_stats.previous_is_first;
    const bool has_optional_starts = ordered_task_stats.has_optional_starts;

    if (_tcsstr(OutBuffer, _T("$(WaypointNext)"))) {
      // Waypoint\nNext
      if (!common_stats.active_has_next)
        invalid = true;

      CondReplaceInString(next_is_final,
                          OutBuffer,
                          _T("$(WaypointNext)"),
                          _("Finish Turnpoint"),
                          _("Next Turnpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointPrevious)"))) {

      if (has_optional_starts && !common_stats.active_has_previous) {
        ReplaceInString(OutBuffer, _T("$(WaypointPrevious)"), _("Next Startpoint"), Size);
      } else {

        CondReplaceInString(previous_is_start,
                            OutBuffer,
                            _T("$(WaypointPrevious)"),
                            _("Start Turnpoint"),
                            _("Previous Turnpoint"), Size);

        if (!common_stats.active_has_previous)
          invalid = true;
      }

    } else if (_tcsstr(OutBuffer, _T("$(WaypointNextArm)"))) {
      // Waypoint\nNext

      switch (task_manager->GetOrderedTask().GetTaskAdvance().GetState()) {
      case TaskAdvance::MANUAL:
      case TaskAdvance::AUTO:
      case TaskAdvance::START_ARMED:
      case TaskAdvance::TURN_ARMED:
        CondReplaceInString(next_is_final,
                            OutBuffer,
                            _T("$(WaypointNextArm)"),
                            _("Finish Turnpoint"),
                            _("Next Turnpoint"), Size);
        if (!common_stats.active_has_next)
          invalid = true;
        break;
      case TaskAdvance::START_DISARMED:
        ReplaceInString(OutBuffer, _T("$(WaypointNextArm)"), _("Arm start"), Size);
        break;
      case TaskAdvance::TURN_DISARMED:
        ReplaceInString(OutBuffer, _T("$(WaypointNextArm)"), _("Arm turn"), Size);
        break;
      }

    } else if (_tcsstr(OutBuffer, _T("$(WaypointPreviousArm)"))) {

      switch (task_manager->GetOrderedTask().GetTaskAdvance().GetState()) {
      case TaskAdvance::MANUAL:
      case TaskAdvance::AUTO:
      case TaskAdvance::START_DISARMED:
      case TaskAdvance::TURN_DISARMED:

        if (has_optional_starts && !common_stats.active_has_previous) {
          ReplaceInString(OutBuffer, _T("$(WaypointPreviousArm)"), _("Next Startpoint"), Size);
        } else {

          CondReplaceInString(previous_is_start,
                              OutBuffer,
                              _T("$(WaypointPreviousArm)"),
                              _("Start Turnpoint"),
                              _("Previous Turnpoint"), Size);

          if (!common_stats.active_has_previous)
            invalid = true;
        }

        break;
      case TaskAdvance::START_ARMED:
        ReplaceInString(OutBuffer, _T("$(WaypointPreviousArm)"), _("Disarm start"), Size);
        break;
      case TaskAdvance::TURN_ARMED:
        ReplaceInString(OutBuffer, _T("$(WaypointPreviousArm)"), _("Disarm turn"), Size);
        break;
      }
    }
  }

  if (_tcsstr(OutBuffer, _T("$(AdvanceArmed)"))) {
    switch (task_manager->GetOrderedTask().GetTaskAdvance().GetState()) {
    case TaskAdvance::MANUAL:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"),
                      _("Advance\n(manual)"), Size);
      invalid = true;
      break;
    case TaskAdvance::AUTO:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"),
                      _("Advance\n(auto)"), Size);
      invalid = true;
      break;
    case TaskAdvance::START_ARMED:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"),
                      _("Abort\nStart"), Size);
      invalid = false;
      break;
    case TaskAdvance::START_DISARMED:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"),
                      _("Arm\nStart"), Size);
      invalid = false;
      break;
    case TaskAdvance::TURN_ARMED:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"),
                      _("Abort\nTurn"), Size);
      invalid = false;
      break;
    case TaskAdvance::TURN_DISARMED:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"),
                      _("Arm\nTurn"), Size);
      invalid = false;
      break;
    }
  }

  if (_tcsstr(OutBuffer, _T("$(CheckAutoMc)"))) {
    if (!task_stats.task_valid
        && settings_computer.task.IsAutoMCFinalGlideEnabled())
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckAutoMc)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(TaskAbortToggleActionName)"))) {
    if (common_stats.task_type == TaskType::GOTO) {
      CondReplaceInString(ordered_task_stats.task_valid,
                          OutBuffer, _T("$(TaskAbortToggleActionName)"),
                          _("Resume"), _("Abort"), Size);
    } else
      CondReplaceInString(common_stats.task_type == TaskType::ABORT,
                          OutBuffer, _T("$(TaskAbortToggleActionName)"),
                          _("Resume"), _("Abort"), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckTaskRestart)"))) {
    if (!(common_stats.task_type == TaskType::ORDERED &&
          task_stats.start.task_started))
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckTaskRestart)"), _T(""), Size);
  }

  return invalid;
}

static void
ExpandTrafficMacros(TCHAR *buffer, size_t size)
{
  TrafficWidget *widget = (TrafficWidget *)
    CommonInterface::main_window->GetFlavourWidget(_T("Traffic"));
  if (widget == nullptr)
    return;

  CondReplaceInString(widget->GetAutoZoom(), buffer,
                      _T("$(TrafficZoomAutoToggleActionName)"),
                      _("Manual"), _("Auto"), size);
  CondReplaceInString(widget->GetNorthUp(), buffer,
                      _T("$(TrafficNorthUpToggleActionName)"),
                      _("Track up"), _("North up"), size);
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

bool
ButtonLabel::ExpandMacros(const TCHAR *In, TCHAR *OutBuffer, size_t Size)
{
  // ToDo, check Buffer Size
  bool invalid = false;
  CopyTruncateString(OutBuffer, Size, In);

  if (_tcsstr(OutBuffer, _T("$(")) == nullptr)
    return false;

  if (_tcsstr(OutBuffer, _T("$(CheckAirspace)"))) {
    if (airspace_database.IsEmpty())
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckAirspace)"), _T(""), Size);
  }

  invalid |= ExpandTaskMacros(OutBuffer, Size,
                              Calculated(), GetComputerSettings());

  ExpandTrafficMacros(OutBuffer, Size);

  if (_tcsstr(OutBuffer, _T("$(CheckFLARM)"))) {
    if (!Basic().flarm.status.available)
      invalid = true;
    ReplaceInString(OutBuffer, _T("$(CheckFLARM)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckWeather)"))) {
    const auto rasp = DataGlobals::GetRasp();
    if (rasp == nullptr || rasp->GetItemCount() == 0)
      invalid = true;
    ReplaceInString(OutBuffer, _T("$(CheckWeather)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckCircling)"))) {
    if (!Calculated().circling)
      invalid = true;
    ReplaceInString(OutBuffer, _T("$(CheckCircling)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckVega)"))) {
    if (devVarioFindVega() == nullptr)
      invalid = true;
    ReplaceInString(OutBuffer, _T("$(CheckVega)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckReplay)"))) {
    if (CommonInterface::MovementDetected())
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckReplay)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckWaypointFile)"))) {
    invalid |= way_points.IsEmpty();
    ReplaceInString(OutBuffer, _T("$(CheckWaypointFile)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckLogger)"))) {
    invalid |= Basic().gps.replay;
    ReplaceInString(OutBuffer, _T("$(CheckLogger)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckNet)"))) {
#ifndef HAVE_HTTP
    invalid = true;
#endif

    ReplaceInString(OutBuffer, _T("$(CheckNet)"), _T(""), Size);
  }
  if (_tcsstr(OutBuffer, _T("$(CheckTerrain)"))) {
    if (!Calculated().terrain_valid)
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckTerrain)"), _T(""), Size);
  }

  CondReplaceInString(logger != nullptr && logger->IsLoggerActive(), OutBuffer,
                      _T("$(LoggerActive)"), _("Stop"),
                      _("Start"), Size);

  if (_tcsstr(OutBuffer, _T("$(SnailTrailToggleName)"))) {
    switch (GetMapSettings().trail.length) {
    case TrailSettings::Length::OFF:
      ReplaceInString(OutBuffer, _T("$(SnailTrailToggleName)"),
                      _("Long"), Size);
      break;
    case TrailSettings::Length::LONG:
      ReplaceInString(OutBuffer, _T("$(SnailTrailToggleName)"),
                      _("Short"), Size);
      break;
    case TrailSettings::Length::SHORT:
      ReplaceInString(OutBuffer, _T("$(SnailTrailToggleName)"),
                      _("Full"), Size);
      break;
    case TrailSettings::Length::FULL:
      ReplaceInString(OutBuffer, _T("$(SnailTrailToggleName)"),
                      _("Off"), Size);
      break;
    }
  }

  if (_tcsstr(OutBuffer, _T("$(AirSpaceToggleName)"))) {
    ReplaceInString(OutBuffer, _T("$(AirSpaceToggleName)"),
                    GetMapSettings().airspace.enable ? _("Off") : _("On"), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(TerrainTopologyToggleName)"))) {
    char val = 0;
    if (GetMapSettings().topography_enabled)
      val++;
    if (GetMapSettings().terrain.enable)
      val += (char)2;
    switch (val) {
    case 0:
      ReplaceInString(OutBuffer, _T("$(TerrainTopologyToggleName)"),
                      _("Topography On"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, _T("$(TerrainTopologyToggleName)"),
                      _("Terrain On"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, _T("$(TerrainTopologyToggleName)"),
                      _("Terrain + Topography"), Size);
      break;
    case 3:
      ReplaceInString(OutBuffer, _T("$(TerrainTopologyToggleName)"),
                      _("Terrain Off"), Size);
      break;
    }
  }

  if (_tcsstr(OutBuffer, _T("$(TerrainTopographyToggleName)"))) {
    char val = 0;
    if (GetMapSettings().topography_enabled)
      val++;
    if (GetMapSettings().terrain.enable)
      val += (char)2;
    switch (val) {
    case 0:
      ReplaceInString(OutBuffer, _T("$(TerrainTopographyToggleName)"),
                      _("Topography On"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, _T("$(TerrainTopographyToggleName)"),
                      _("Terrain On"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, _T("$(TerrainTopographyToggleName)"),
                      _("Terrain + Topography"), Size);
      break;
    case 3:
      ReplaceInString(OutBuffer, _T("$(TerrainTopographyToggleName)"),
                      _("Terrain Off"), Size);
      break;
    }
  }

  CondReplaceInString(CommonInterface::main_window->GetFullScreen(), OutBuffer,
                      _T("$(FullScreenToggleActionName)"),
                      _("Off"), _("On"), Size);
  CondReplaceInString(GetMapSettings().auto_zoom_enabled, OutBuffer,
                      _T("$(ZoomAutoToggleActionName)"),
                      _("Manual"), _("Auto"), Size);
  CondReplaceInString(GetMapSettings().topography_enabled, OutBuffer,
                      _T("$(TopologyToggleActionName)"),
                      _("Off"), _("On"), Size);
  CondReplaceInString(GetMapSettings().topography_enabled, OutBuffer,
                      _T("$(TopographyToggleActionName)"),
                      _("Off"), _("On"), Size);
  CondReplaceInString(GetMapSettings().terrain.enable, OutBuffer,
                      _T("$(TerrainToggleActionName)"),
                      _("Off"), _("On"), Size);
  CondReplaceInString(GetMapSettings().airspace.enable, OutBuffer,
                      _T("$(AirspaceToggleActionName)"),
                      _("Off"), _("On"), Size);

  if (_tcsstr(OutBuffer, _T("$(MapLabelsToggleActionName)"))) {
    static const TCHAR *const labels[] = {
      N_("All"),
      N_("Task & Landables"),
      N_("Task"),
      N_("None"),
      N_("Task & Airfields"),
    };

    static constexpr unsigned int n = ARRAY_SIZE(labels);
    unsigned int i = (unsigned)GetMapSettings().waypoint.label_selection;
    ReplaceInString(OutBuffer, _T("$(MapLabelsToggleActionName)"),
                    gettext(labels[(i + 1) % n]), Size);
  }

  CondReplaceInString(GetComputerSettings().task.auto_mc,
                      OutBuffer, _T("$(MacCreadyToggleActionName)"),
                      _("Manual"), _("Auto"), Size);
  CondReplaceInString(GetUIState().auxiliary_enabled,
                      OutBuffer, _T("$(AuxInfoToggleActionName)"),
                      _("Off"), _("On"), Size);

  CondReplaceInString(GetUIState().force_display_mode == DisplayMode::CIRCLING,
                      OutBuffer, _T("$(DispModeClimbShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetUIState().force_display_mode == DisplayMode::CRUISE,
                      OutBuffer, _T("$(DispModeCruiseShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetUIState().force_display_mode == DisplayMode::NONE,
                      OutBuffer, _T("$(DispModeAutoShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetUIState().force_display_mode == DisplayMode::FINAL_GLIDE,
                      OutBuffer, _T("$(DispModeFinalShortIndicator)"),
                      _T("(*)"), _T(""), Size);

  CondReplaceInString(GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::ALLON,
                      OutBuffer, _T("$(AirspaceModeAllShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::CLIP,
                      OutBuffer, _T("$(AirspaceModeClipShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::AUTO,
                      OutBuffer, _T("$(AirspaceModeAutoShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::ALLBELOW,
                      OutBuffer, _T("$(AirspaceModeBelowShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::ALLOFF,
                      OutBuffer, _T("$(AirspaceModeAllOffIndicator)"),
                      _T("(*)"), _T(""), Size);

  CondReplaceInString(GetMapSettings().trail.length == TrailSettings::Length::OFF,
                      OutBuffer, _T("$(SnailTrailOffShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetMapSettings().trail.length == TrailSettings::Length::SHORT,
                      OutBuffer, _T("$(SnailTrailShortShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetMapSettings().trail.length == TrailSettings::Length::LONG,
                      OutBuffer, _T("$(SnailTrailLongShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetMapSettings().trail.length == TrailSettings::Length::FULL,
                      OutBuffer, _T("$(SnailTrailFullShortIndicator)"),
                      _T("(*)"), _T(""), Size);

  CondReplaceInString(!GetMapSettings().airspace.enable,
                      OutBuffer, _T("$(AirSpaceOffShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(GetMapSettings().airspace.enable,
                      OutBuffer, _T("$(AirSpaceOnShortIndicator)"),
                      _T("(*)"), _T(""), Size);

  CondReplaceInString(CommonInterface::GetUISettings().traffic.enable_gauge,
                      OutBuffer, _T("$(FlarmDispToggleActionName)"),
                      _("Off"), _("On"), Size);

  CondReplaceInString(GetMapSettings().auto_zoom_enabled, OutBuffer,
                      _T("$(ZoomAutoToggleActionName)"),
                      _("Manual"), _("Auto"), Size);

  if (_tcsstr(OutBuffer, _T("$(NextPageName)"))) {
    TCHAR label[30];
    const PageLayout &page =
      CommonInterface::GetUISettings().pages.pages[PageActions::NextIndex()];
    page.MakeTitle(CommonInterface::GetUISettings().info_boxes, label, true);
    ReplaceInString(OutBuffer, _T("$(NextPageName)"), label, Size);
  }

  return invalid;
}
