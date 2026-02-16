// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Menu/ButtonLabel.hpp"
#include "Language/Language.hpp"
#include "Logger/Logger.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Gauge/BigTrafficWidget.hpp"
#include "Computer/Settings.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "DataGlobals.hpp"
#include "MapSettings.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Device/MultipleDevices.hpp"
#include "PageActions.hpp"
#include "util/DollarExpand.hpp"
#include "util/Macros.hpp"
#include "net/http/Features.hpp"
#include "UIState.hpp"

#include <stdlib.h>

static const char *
ExpandTaskMacros(std::string_view name,
                 bool &invalid,
                 const DerivedInfo &calculated,
                 const ComputerSettings &settings_computer) noexcept
{
  const TaskStats &task_stats = calculated.task_stats;
  const TaskStats &ordered_task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  if (name == "CheckTaskResumed") {
    // TODO code: check, does this need to be set with temporary task?
    invalid |= common_stats.task_type == TaskType::ABORT ||
      common_stats.task_type == TaskType::GOTO;
    return "";
  } else if (name == "CheckTask") {
    invalid |= !task_stats.task_valid;
    return "";
  }

  if (!backend_components->protected_task_manager) {
    invalid = true;
    return nullptr;
  }

  ProtectedTaskManager::Lease task_manager{*backend_components->protected_task_manager};

  const AbstractTask *task = task_manager->GetActiveTask();
  if (task == nullptr || !task_stats.task_valid ||
      common_stats.task_type == TaskType::GOTO) {

    if (name == "WaypointNext" ||
        name == "WaypointNextArm") {
      invalid = true;
      return _("Next Turnpoint");
    } else if (name == "WaypointPrevious" ||
               name == "WaypointPreviousArm") {
      invalid = true;
      return _("Previous Turnpoint");
    }
  } else if (common_stats.task_type == TaskType::ABORT) {
    if (name == "WaypointNext" ||
        name == "WaypointNextArm") {
      invalid |= !common_stats.active_has_next;
      return common_stats.next_is_last
        ? _("Furthest Landpoint")
        : _("Next Landpoint");
    } else if (name == "WaypointPrevious" ||
               name == "WaypointPreviousArm") {
      invalid |= !common_stats.active_has_previous;

      return common_stats.previous_is_first
        ? _("Closest Landpoint")
        : _("Previous Landpoint");
    }

  } else { // ordered task mode

    const bool next_is_final = common_stats.next_is_last;
    const bool previous_is_start = common_stats.previous_is_first;
    const bool has_optional_starts = ordered_task_stats.has_optional_starts;

    if (name == "WaypointNext") {
      // Waypoint\nNext
      invalid |= !common_stats.active_has_next;
      return next_is_final
        ? _("Finish Turnpoint")
        : _("Next Turnpoint");
    } else if (name == "WaypointPrevious") {
      if (has_optional_starts && !common_stats.active_has_previous) {
        return _("Next Startpoint");
      } else {
        invalid |= !common_stats.active_has_previous;
        return previous_is_start
          ? _("Start Turnpoint")
          : _("Previous Turnpoint");
      }

    } else if (name == "WaypointNextArm") {
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

    } else if (name == "WaypointPreviousArm") {

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

  if (name == "AdvanceArmed") {
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
  } else if (name == "CheckAutoMc") {
    invalid |= !task_stats.task_valid &&
      settings_computer.task.IsAutoMCFinalGlideEnabled();
    return "";
  } else if (name == "TaskAbortToggleActionName") {
    if (common_stats.task_type == TaskType::GOTO)
      return ordered_task_stats.task_valid
        ? _("Resume")
        : _("Abort");
    else
      return common_stats.task_type == TaskType::ABORT
        ? _("Resume")
        : _("Abort");
  } else if (name == "CheckTaskRestart") {
    invalid |= !(common_stats.task_type == TaskType::ORDERED &&
                 task_stats.start.HasStarted());
    return "";
  }

  return nullptr;
}

[[gnu::pure]]
static const char *
ExpandTrafficMacros(std::string_view name) noexcept
{
  TrafficWidget *widget = (TrafficWidget *)
    CommonInterface::main_window->GetFlavourWidget("Traffic");
  if (widget == nullptr)
    return nullptr;

  if (name == "TrafficZoomAutoToggleActionName")
    return widget->GetAutoZoom() ? _("Manual") : _("Auto");
  else if (name == "TrafficNorthUpToggleActionName")
    return widget->GetNorthUp() ? _("Track up") : _("North up");
  else
    return nullptr;
}

static const NMEAInfo &
Basic() noexcept
{
  return CommonInterface::Basic();
}

static const DerivedInfo &
Calculated() noexcept
{
  return CommonInterface::Calculated();
}

static const ComputerSettings &
GetComputerSettings() noexcept
{
  return CommonInterface::GetComputerSettings();
}

static const MapSettings &
GetMapSettings() noexcept
{
  return CommonInterface::GetMapSettings();
}

static const UIState &
GetUIState() noexcept
{
  return CommonInterface::GetUIState();
}

static const char *
LookupMacro(std::string_view name, bool &invalid) noexcept
{
  if (name =="CheckAirspace") {
    invalid |= data_components->airspaces->IsEmpty();
    return nullptr;
  }

  const char *value = ExpandTaskMacros(name, invalid,
                                        Calculated(), GetComputerSettings());
  if (value != nullptr)
    return value;

  value = ExpandTrafficMacros(name);
  if (value != nullptr)
    return value;

  if (name =="CheckFLARM") {
    invalid |= !Basic().flarm.status.available;
    return nullptr;
  } else if (name == "CheckWeather") {
    const auto rasp = DataGlobals::GetRasp();
    invalid |= rasp == nullptr || rasp->GetItemCount() == 0;
    return nullptr;
  } else if (name == "CheckCircling") {
    invalid |= !Calculated().circling;
    return nullptr;
  } else if (name == "CheckVega") {
    invalid |= backend_components->devices == nullptr ||
      !backend_components->devices->HasVega();
    return nullptr;
  } else if (name == "CheckReplay") {
    invalid |= CommonInterface::MovementDetected();
    return nullptr;
  } else if (name == "CheckWaypointFile") {
    invalid |= data_components->waypoints->IsEmpty();
    return nullptr;
  } else if (name == "CheckLogger") {
    invalid |= Basic().gps.replay;
    return nullptr;
  } else if (name == "CheckNet") {
#ifndef HAVE_HTTP
    invalid = true;
#endif
    return nullptr;
  } else if (name == "CheckTerrain") {
    invalid |= !Calculated().terrain_valid;
    return nullptr;
  } else if (name == "LoggerActive") {
    return backend_components->igc_logger != nullptr &&
      backend_components->igc_logger->IsLoggerActive()
      ? _("Stop")
      : _("Start");
  } else if (name == "SnailTrailToggleName") {
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
  } else if (name == "AirSpaceToggleName") {
    return GetMapSettings().airspace.enable ? _("Off") : _("On");
  } else if (name == "TerrainTopologyToggleName" ||
             name == "TerrainTopographyToggleName") {
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
  } else if (name == "FullScreenToggleActionName") {
    return CommonInterface::main_window->GetFullScreen() ? _("Off") : _("On");
  } else if (name == "ZoomAutoToggleActionName") {
    return GetMapSettings().auto_zoom_enabled ? _("Manual") : _("Auto");
  } else if (name == "TopologyToggleActionName" ||
             name == "TopographyToggleActionName") {
    return GetMapSettings().topography_enabled ? _("Hide") : _("Show");
  } else if (name == "TerrainToggleActionName") {
    return GetMapSettings().terrain.enable ? _("Hide") : _("Show");
  } else if (name == "AirspaceToggleActionName") {
    return GetMapSettings().airspace.enable ? _("Hide") : _("Show");
  } else if (name == "MapLabelsToggleActionName") {
    static const char *const labels[] = {
      N_("All"),
      N_("Task & Landables"),
      N_("Task"),
      N_("None"),
      N_("Task & Airfields"),
    };

    static constexpr unsigned int n = ARRAY_SIZE(labels);
    unsigned int i = (unsigned)GetMapSettings().waypoint.label_selection;
    return gettext(labels[(i + 1) % n]);
  } else if (name == "MacCreadyToggleActionName") {
    return GetComputerSettings().task.auto_mc ? _("Manual") : _("Auto");
  } else if (name == "AuxInfoToggleActionName") {
    return GetUIState().auxiliary_enabled ? _("Off") : _("On");
  } else if (name == "DispModeClimbShortIndicator") {
    return GetUIState().force_display_mode == DisplayMode::CIRCLING
      ? "*" : "";
  } else if (name == "DispModeCruiseShortIndicator") {
    return GetUIState().force_display_mode == DisplayMode::CRUISE
      ? "*" : "";
  } else if (name == "DispModeAutoShortIndicator") {
    return GetUIState().force_display_mode == DisplayMode::NONE
      ? "*" : "";
  } else if (name == "DispModeFinalShortIndicator") {
    return GetUIState().force_display_mode == DisplayMode::FINAL_GLIDE
      ? "*" : "";
  } else if (name == "AirspaceModeAllShortIndicator") {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::ALLON
      ? "*" : "";
  } else if (name == "AirspaceModeClipShortIndicator") {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::CLIP
      ? "*" : "";
  } else if (name == "AirspaceModeAutoShortIndicator") {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::AUTO
      ? "*" : "";
  } else if (name == "AirspaceModeBelowShortIndicator") {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::ALLBELOW
      ? "*" : "";
  } else if (name == "AirspaceModeAllOffIndicator") {
    return GetMapSettings().airspace.altitude_mode == AirspaceDisplayMode::ALLOFF
      ? "*" : "";
  } else if (name == "SnailTrailOffShortIndicator") {
    return GetMapSettings().trail.length == TrailSettings::Length::OFF
      ? "*" : "";
  } else if (name == "SnailTrailShortShortIndicator") {
    return GetMapSettings().trail.length == TrailSettings::Length::SHORT
      ? "*" : "";
  } else if (name == "SnailTrailLongShortIndicator") {
    return GetMapSettings().trail.length == TrailSettings::Length::LONG
      ? "*" : "";
  } else if (name == "SnailTrailFullShortIndicator") {
    return GetMapSettings().trail.length == TrailSettings::Length::FULL
      ? "*" : "";
  } else if (name == "AirSpaceOffShortIndicator") {
    return !GetMapSettings().airspace.enable ? "*" : "";
  } else if (name == "AirSpaceOnShortIndicator") {
    return GetMapSettings().airspace.enable ? "*" : "";
  } else if (name == "FlarmDispToggleActionName") {
    return CommonInterface::GetUISettings().traffic.enable_gauge
      ? _("Off") : _("On");
  } else if (name == "ZoomAutoToggleActionName") {
    return GetMapSettings().auto_zoom_enabled ? _("Manual") : _("Auto");
  } else if (name == "NextPageName") {
    static char label[64]; // TODO: oh no, a static string buffer!
    const PageLayout &page =
      CommonInterface::GetUISettings().pages.pages[PageActions::NextIndex()];
    return page.MakeTitle(CommonInterface::GetUISettings().info_boxes,
                          std::span{label}, true);
  } else if (name == "CheckWeGlide") {
    invalid |= !CommonInterface::GetComputerSettings().weglide.enabled;
    return nullptr;
  } else
    return nullptr;
}

bool
ButtonLabel::ExpandMacros(const char *In, std::span<char> dest) noexcept
{
  bool invalid = false;

  DollarExpand(In, dest,
               [&invalid](std::string_view name){
                 return LookupMacro(name, invalid);
               });

  return invalid;
}
