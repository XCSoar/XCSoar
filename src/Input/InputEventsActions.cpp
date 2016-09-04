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

/*

InputEvents

This class is used to control all user and external InputEvents.
This includes some Nmea strings, virtual events (Glide Computer
Evnets) and Keyboard.

What it does not cover is Glide Computer normal processing - this
includes GPS and Vario processing.

What it does include is what to do when an automatic event (switch
to Climb mode) and user events are entered.

It also covers the configuration side of on screen labels.

For further information on config file formats see

source/Common/Data/Input/ALL
doc/html/advanced/input/ALL		http://xcsoar.sourceforge.net/advanced/input/

*/

#include "InputEvents.hpp"
#include "Protection.hpp"
#include "UIState.hpp"
#include "Computer/Settings.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Error.hpp"
#include "Dialogs/Device/Vega/SwitchesDialog.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Dialogs/Traffic/TrafficDialogs.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/Weather/WeatherDialog.hpp"
#include "Dialogs/Plane/PlaneDialogs.hpp"
#include "Dialogs/ProfileListDialog.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "Dialogs/FileManager.hpp"
#include "Dialogs/ReplayDialog.hpp"
#include "Message.hpp"
#include "Markers/Markers.hpp"
#include "MainWindow.hpp"
#include "PopupMessage.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Audio/Sound.hpp"
#include "UIActions.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Components.hpp"
#include "Language/Language.hpp"
#include "Logger/Logger.hpp"
#include "Logger/NMEALogger.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/Factory.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "UtilsSettings.hpp"
#include "PageActions.hpp"
#include "Compiler.h"
#include "MapWindow/GlueMapWindow.hpp"
#include "Simulator.hpp"
#include "Formatter/TimeFormatter.hpp"

#include <assert.h>
#include <tchar.h>
#include <algorithm>

/**
 * Determine the reference location of the current map display.
 */
gcc_pure
static GeoPoint
GetVisibleLocation()
{
  const auto &projection = CommonInterface::main_window->GetProjection();
  if (projection.IsValid())
    return projection.GetGeoLocation();

  /* just in case the Projection is broken for whatever reason, fall
     back to NMEAInfo::location */

  const auto &basic = CommonInterface::Basic();
  if (basic.location_available)
    return basic.location;

  return GeoPoint::Invalid();
}

static void
trigger_redraw()
{
  if (!CommonInterface::Basic().location_available)
    ForceCalculation();
  TriggerMapUpdate();
}

/**
 * Wrapper for #ScopeSuspendAllThreads and Waypoints::Append().
 *
 * @return a reference to the #Waypoint stored in #Waypoints
 */
static WaypointPtr
SuspendAppendWaypoint(Waypoint &&wp)
{
  ScopeSuspendAllThreads suspend;
  auto ptr = way_points.Append(std::move(wp));
  way_points.Optimise();
  return ptr;
}

static WaypointPtr
SuspendAppendSaveWaypoint(Waypoint &&wp)
{
  auto ptr = SuspendAppendWaypoint(std::move(wp));

  try {
    WaypointGlue::SaveWaypoint(*ptr);
  } catch (const std::runtime_error &e) {
    ShowError(e, _("Failed to save waypoints"));
  }

  return ptr;
}

// -----------------------------------------------------------------------
// Execution - list of things you can do
// -----------------------------------------------------------------------

// TODO code: Keep marker text for use in log file etc.
void
InputEvents::eventMarkLocation(const TCHAR *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (StringIsEqual(misc, _T("reset"))) {
    ScopeSuspendAllThreads suspend;
    way_points.EraseUserMarkers();
  } else {
    const auto location = GetVisibleLocation();
    if (!location.IsValid())
      return;

    MarkLocation(location, basic.date_time_utc);

    const WaypointFactory factory(WaypointOrigin::USER, terrain);
    Waypoint wp = factory.Create(location);
    factory.FallbackElevation(wp);

    TCHAR name[64] = _T("Marker");
    if (basic.date_time_utc.IsPlausible()) {
      auto *p = name + StringLength(name);
      *p++ = _T(' ' );
      FormatISO8601(p, basic.date_time_utc);
    }

    wp.name = name;
    wp.type = Waypoint::Type::MARKER;

    SuspendAppendSaveWaypoint(std::move(wp));

    if (CommonInterface::GetUISettings().sound.sound_modes_enabled)
      PlayResource(_T("IDR_WAV_CLEAR"));
  }

  trigger_redraw();
}

void
InputEvents::eventScreenModes(const TCHAR *misc)
{
  // toggle switches like this:
  //  -- normal infobox
  //  -- auxiliary infobox
  //  -- full screen
  //  -- normal infobox

  const UIState &ui_state = CommonInterface::GetUIState();

  if (StringIsEqual(misc, _T("normal"))) {
    PageActions::OpenLayout(PageLayout::Default());
  } else if (StringIsEqual(misc, _T("auxilary"))) {
    PageActions::OpenLayout(PageLayout::Aux());
  } else if (StringIsEqual(misc, _T("toggleauxiliary"))) {
    PageActions::OpenLayout(ui_state.auxiliary_enabled
                            ? PageLayout::Default()
                            : PageLayout::Aux());
  } else if (StringIsEqual(misc, _T("full"))) {
    PageActions::OpenLayout(PageLayout::FullScreen());
  } else if (StringIsEqual(misc, _T("togglefull"))) {
    CommonInterface::main_window->SetFullScreen(
        !CommonInterface::main_window->GetFullScreen());
  } else if (StringIsEqual(misc, _T("show"))) {
    if (CommonInterface::main_window->GetFullScreen())
      Message::AddMessage(_("Screen Mode Full"));
    else if (ui_state.auxiliary_enabled)
        Message::AddMessage(_("Auxiliary InfoBoxes"));
    else
        Message::AddMessage(_("Default InfoBoxes"));
  } else if (StringIsEqual(misc, _T("previous")))
    PageActions::Prev();
  else
    PageActions::Next();


  trigger_redraw();
}

// ClearStatusMessages
// Do Clear Event Warnings
void
InputEvents::eventClearStatusMessages(gcc_unused const TCHAR *misc)
{
  // TODO enhancement: allow selection of specific messages (here we are acknowledging all)
  if (CommonInterface::main_window->popup != nullptr)
    CommonInterface::main_window->popup->Acknowledge();
}

// Mode
// Sets the current event mode.
//  The argument is the label of the mode to activate.
//  This is used to activate menus/submenus of buttons
void
InputEvents::eventMode(const TCHAR *misc)
{
  assert(misc != NULL);

  CommonInterface::main_window->SetDefaultFocus();

  InputEvents::setMode(misc);
}

// Don't think we need this.
void
InputEvents::eventMainMenu(gcc_unused const TCHAR *misc)
{
  // todo: popup main menu
}

// Checklist
// Displays the checklist dialog
//  See the checklist dialog section of the reference manual for more info.
void
InputEvents::eventChecklist(gcc_unused const TCHAR *misc)
{
  dlgChecklistShowModal();
}

// Status
// Displays one of the three status dialogs:
//    system: display the system status
//    aircraft: displays the aircraft status
//    task: displays the task status
//  See the status dialog section of the reference manual for more info
//  on these.
void
InputEvents::eventStatus(const TCHAR *misc)
{
  if (StringIsEqual(misc, _T("system"))) {
    dlgStatusShowModal(1);
  } else if (StringIsEqual(misc, _T("task"))) {
    dlgStatusShowModal(2);
  } else if (StringIsEqual(misc, _T("aircraft"))) {
    dlgStatusShowModal(0);
  } else {
    dlgStatusShowModal(-1);
  }
}

// Analysis
// Displays the analysis/statistics dialog
//  See the analysis dialog section of the reference manual
// for more info.
void
InputEvents::eventAnalysis(gcc_unused const TCHAR *misc)
{
  dlgAnalysisShowModal(*CommonInterface::main_window,
                       CommonInterface::main_window->GetLook(),
                       CommonInterface::Full(),
                       *glide_computer,
                       &airspace_database,
                       terrain);
}

// WaypointDetails
// Displays waypoint details
//         current: the current active waypoint
//          select: brings up the waypoint selector, if the user then
//                  selects a waypoint, then the details dialog is shown.
//  See the waypoint dialog section of the reference manual
// for more info.
void
InputEvents::eventWaypointDetails(const TCHAR *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  WaypointPtr wp;

  bool allow_navigation = true;
  bool allow_edit = true;

  if (StringIsEqual(misc, _T("current"))) {
    if (protected_task_manager == NULL)
      return;

    wp = protected_task_manager->GetActiveWaypoint();
    if (!wp) {
      Message::AddMessage(_("No active waypoint!"));
      return;
    }

    /* due to a code limitation, we can't currently manipulate
       Waypoint instances taken from the task, because it would
       require updating lots of internal task state, and the waypoint
       editor doesn't know how to do that */
    allow_navigation = false;
    allow_edit = false;
  } else if (StringIsEqual(misc, _T("select"))) {
    wp = ShowWaypointListDialog(basic.location);
  }
  if (wp)
    dlgWaypointDetailsShowModal(std::move(wp),
                                allow_navigation, allow_edit);
}

void
InputEvents::eventWaypointEditor(const TCHAR *misc)
{
  dlgConfigWaypointsShowModal();
}

// StatusMessage
// Displays a user defined status message.
//    The argument is the text to be displayed.
//    No punctuation characters are allowed.
void
InputEvents::eventStatusMessage(const TCHAR *misc)
{
  if (misc != NULL)
    Message::AddMessage(gettext(misc));
}

// Plays a sound from the filename
void
InputEvents::eventPlaySound(const TCHAR *misc)
{
  PlayResource(misc);
}

void
InputEvents::eventAutoLogger(const TCHAR *misc)
{
  if (is_simulator())
    return;

  LoggerSettings::AutoLogger auto_logger =
    CommonInterface::GetComputerSettings().logger.auto_logger;

  if (auto_logger == LoggerSettings::AutoLogger::OFF)
    return;

  if (auto_logger == LoggerSettings::AutoLogger::START_ONLY &&
      !StringIsEqual(misc, _T("start")))
    return;

  eventLogger(misc);
}

// Logger
// Activates the internal IGC logger
//  start: starts the logger
// start ask: starts the logger after asking the user to confirm
// stop: stops the logger
// stop ask: stops the logger after asking the user to confirm
// toggle: toggles between on and off
// toggle ask: toggles between on and off, asking the user to confirm
// show: displays a status message indicating whether the logger is active
// nmea: turns on and off NMEA logging
// note: the text following the 'note' characters is added to the log file
void
InputEvents::eventLogger(const TCHAR *misc)
{
  if (logger == nullptr)
    return;

  // TODO feature: start logger without requiring feedback
  // start stop toggle addnote

  const NMEAInfo &basic = CommonInterface::Basic();
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  if (StringIsEqual(misc, _T("start ask")))
    logger->GUIStartLogger(basic, settings_computer,
                           protected_task_manager);
  else if (StringIsEqual(misc, _T("start")))
    logger->GUIStartLogger(basic, settings_computer,
                           protected_task_manager, true);
  else if (StringIsEqual(misc, _T("stop ask")))
    logger->GUIStopLogger(basic);
  else if (StringIsEqual(misc, _T("stop")))
    logger->GUIStopLogger(basic, true);
  else if (StringIsEqual(misc, _T("toggle ask")))
    logger->GUIToggleLogger(basic, settings_computer,
                            protected_task_manager);
  else if (StringIsEqual(misc, _T("toggle")))
    logger->GUIToggleLogger(basic, settings_computer,
                            protected_task_manager, true);
  else if (StringIsEqual(misc, _T("nmea"))) {
    NMEALogger::enabled = !NMEALogger::enabled;
    if (NMEALogger::enabled) {
      Message::AddMessage(_("NMEA log on"));
    } else {
      Message::AddMessage(_("NMEA log off"));
    }
  } else if (StringIsEqual(misc, _T("show")))
    if (logger->IsLoggerActive()) {
      Message::AddMessage(_("Logger on"));
    } else {
      Message::AddMessage(_("Logger off"));
    }
  else if (StringIsEqual(misc, _T("note"), 4))
    // add note to logger file if available..
    logger->LoggerNote(misc + 4);
}

// RepeatStatusMessage
// Repeats the last status message.  If pressed repeatedly, will
// repeat previous status messages
void
InputEvents::eventRepeatStatusMessage(gcc_unused const TCHAR *misc)
{
  // new interface
  // TODO enhancement: display only by type specified in misc field
  if (CommonInterface::main_window->popup != nullptr)
    CommonInterface::main_window->popup->Repeat();
}

// NearestWaypointDetails
// Displays the waypoint details dialog
void
InputEvents::eventNearestWaypointDetails(gcc_unused const TCHAR *misc)
{
  const auto location = GetVisibleLocation();
  if (!location.IsValid())
    return;

  // big range..
  PopupNearestWaypointDetails(way_points, location, 1.0e5);
}

// NearestMapItems
// Displays the map item list dialog
void
InputEvents::eventNearestMapItems(gcc_unused const TCHAR *misc)
{
  const auto location = GetVisibleLocation();
  if (!location.IsValid())
    return;

  CommonInterface::main_window->GetMap()->ShowMapItems(location);
}

// Null
// The null event does nothing.  This can be used to override
// default functionality
void
InputEvents::eventNull(gcc_unused const TCHAR *misc)
{
  // do nothing
}

void
InputEvents::eventBeep(gcc_unused const TCHAR *misc)
{
#ifdef WIN32
  MessageBeep(MB_ICONEXCLAMATION);
#else
  PlayResource(_T("IDR_WAV_CLEAR"));
  #endif
}

// Setup
// Activates configuration and setting dialogs
//  Basic: Basic settings (QNH/Bugs/Ballast/MaxTemperature)
//  Wind: Wind settings
//  Task: Task editor
//  Airspace: Airspace filter settings
//  Replay: IGC replay dialog
void
InputEvents::eventSetup(const TCHAR *misc)
{
  if (StringIsEqual(misc, _T("Basic")))
    dlgBasicSettingsShowModal();
  else if (StringIsEqual(misc, _T("Wind")))
    ShowWindSettingsDialog();
  else if (StringIsEqual(misc, _T("System")))
    SystemConfiguration();
  else if (StringIsEqual(misc, _T("Task")))
    dlgTaskManagerShowModal();
  else if (StringIsEqual(misc, _T("Airspace")))
    dlgAirspaceShowModal(false);
  else if (StringIsEqual(misc, _T("Weather")))
    ShowWeatherDialog(_T("rasp"));
  else if (StringIsEqual(misc, _T("Replay"))) {
    if (!CommonInterface::MovementDetected())
      ShowReplayDialog();
  } else if (StringIsEqual(misc, _T("Switches")))
    dlgSwitchesShowModal();
  else if (StringIsEqual(misc, _T("Teamcode")))
    dlgTeamCodeShowModal();
  else if (StringIsEqual(misc, _T("Target")))
    dlgTargetShowModal();
  else if (StringIsEqual(misc, _T("Plane")))
    dlgPlanesShowModal();
  else if (StringIsEqual(misc, _T("Profile")))
    ProfileListDialog();
  else if (StringIsEqual(misc, _T("Alternates")))
    dlgAlternatesListShowModal();

  trigger_redraw();
}

void
InputEvents::eventCredits(gcc_unused const TCHAR *misc)
{
  dlgCreditsShowModal(*CommonInterface::main_window);
}

// Run
// Runs an external program of the specified filename.
// Note that XCSoar will wait until this program exits.
void
InputEvents::eventRun(const TCHAR *misc)
{
  #ifdef WIN32
  PROCESS_INFORMATION pi;
  if (!::CreateProcess(misc, NULL, NULL, NULL, false, 0, NULL, NULL, NULL, &pi))
    return;

  // wait for program to finish!
  ::WaitForSingleObject(pi.hProcess, INFINITE);
  ::CloseHandle(pi.hProcess);
  ::CloseHandle(pi.hThread);

  #else /* !WIN32 */
  system(misc);
  #endif /* !WIN32 */
}

void
InputEvents::eventBrightness(gcc_unused const TCHAR *misc)
{
  // not implemented (was only implemented on Altair)
}

void
InputEvents::eventExit(gcc_unused const TCHAR *misc)
{
  UIActions::SignalShutdown(false);
}

void
InputEvents::eventUserDisplayModeForce(const TCHAR *misc)
{
  UIState &ui_state = CommonInterface::SetUIState();

  if (StringIsEqual(misc, _T("unforce")))
    ui_state.force_display_mode = DisplayMode::NONE;
  else if (StringIsEqual(misc, _T("forceclimb")))
    ui_state.force_display_mode = DisplayMode::CIRCLING;
  else if (StringIsEqual(misc, _T("forcecruise")))
    ui_state.force_display_mode = DisplayMode::CRUISE;
  else if (StringIsEqual(misc, _T("forcefinal")))
    ui_state.force_display_mode = DisplayMode::FINAL_GLIDE;

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState();
}

void
InputEvents::eventAddWaypoint(const TCHAR *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (StringIsEqual(misc, _T("takeoff"))) {
    if (basic.location_available && calculated.terrain_valid) {
      ScopeSuspendAllThreads suspend;
      way_points.AddTakeoffPoint(basic.location, calculated.terrain_altitude);
      way_points.Optimise();
    }
  } else {
    Waypoint edit_waypoint = way_points.Create(basic.location);
    edit_waypoint.elevation = calculated.terrain_altitude;
    if (!dlgWaypointEditShowModal(edit_waypoint) || edit_waypoint.name.empty()) {
      trigger_redraw();
      return;
    }

    SuspendAppendSaveWaypoint(std::move(edit_waypoint));
  }

  trigger_redraw();
}

// JMW TODO enhancement: have all inputevents return bool, indicating whether
// the button should after processing be hilit or not.
// this allows the buttons to indicate whether things are enabled/disabled
// SDP TODO enhancement: maybe instead do conditional processing ?
//     I like this idea; if one returns false, then don't execute the
//     remaining events.

// JMW TODO enhancement: make sure when we change things here we also set registry values...
// or maybe have special tag "save" which indicates it should be saved (notice that
// the wind adjustment uses this already, see in Process.cpp)

/* Recently done

eventTaskLoad		- Load tasks from a file (misc = filename)
eventTaskSave		- Save tasks to a file (misc = filename)
eventProfileLoad		- Load profile from a file (misc = filename)
eventProfileSave		- Save profile to a file (misc = filename)

*/

/* TODO feature: - new events

eventPanWaypoint		                - Set pan to a waypoint
- Waypoint could be "next", "first", "last", "previous", or named
- Note: wrong name - probably just part of eventPan
eventPressure		- Increase, Decrease, show, Set pressure value
eventDeclare			- (JMW separate from internal logger)
eventAirspaceDisplay	- all, below nnn, below me, auto nnn
eventAirspaceWarnings- on, off, time nn, ack nn
eventTerrain			- see map_window.Event_Terrain
eventCompass			- on, off, cruise on, crusie off, climb on, climb off
eventVario			- on, off // JMW what does this do?
eventOrientation		- north, track,  ???
eventTerrainRange	        - on, off (might be part of eventTerrain)
eventSounds			- Include Task and Modes sounds along with Vario
- Include master nn, deadband nn, netto trigger mph/kts/...

*/

// helpers

void
InputEvents::eventWeather(const TCHAR *misc)
{
  ShowWeatherDialog(misc);
}

void
InputEvents::eventQuickMenu(gcc_unused const TCHAR *misc)
{
 dlgQuickMenuShowModal(*CommonInterface::main_window);
}

void
InputEvents::eventFileManager(const TCHAR *misc)
{
  ShowFileManager();
}
