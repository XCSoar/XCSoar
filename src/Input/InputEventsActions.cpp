// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
doc/html/advanced/input/ALL
https://xcsoar.readthedocs.io/en/latest/input_events.html

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
#include "Dialogs/DataManagement/DataManagement.hpp"
#include "Dialogs/DataManagement/ExportFlightsPanel.hpp"
#include "Dialogs/ProfileListDialog.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "Dialogs/FileManager.hpp"
#include "Dialogs/ReplayDialog.hpp"
#include "Dialogs/dlgQuickGuide.hpp"
#include "Dialogs/dlgGestureHelp.hpp"
#include "Message.hpp"
#include "Markers/Markers.hpp"
#include "MainWindow.hpp"
#include "PopupMessage.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Audio/Sound.hpp"
#include "UIActions.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "Logger/Logger.hpp"
#include "Logger/NMEALogger.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/Factory.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "UtilsSettings.hpp"
#include "PageActions.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Simulator.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Device/MultipleDevices.hpp"
#include "Form/DataField/File.hpp"
#include "Dialogs/FilePicker.hpp"
#include "net/client/WeGlide/UploadIGCFile.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Terrain/RasterTerrain.hpp"

#include <cassert>
#include <algorithm>
#include <limits>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#ifdef _WIN32
#include <processthreadsapi.h> // for CreateProcess()
#include <winbase.h> // for INFINITE
#endif

/**
 * Determine the reference location of the current map display.
 */
[[gnu::pure]]
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
  auto &way_points = *data_components->waypoints;
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
  } catch (...) {
    ShowError(std::current_exception(), _("Failed to save waypoints"));
  }

  return ptr;
}

// -----------------------------------------------------------------------
// Execution - list of things you can do
// -----------------------------------------------------------------------

// TODO code: Keep marker text for use in log file etc.
void
InputEvents::eventMarkLocation(const char *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (StringIsEqual(misc, "reset")) {
    ScopeSuspendAllThreads suspend;
    data_components->waypoints->EraseUserMarkers();
  } else {
    const auto location = GetVisibleLocation();
    if (!location.IsValid())
      return;

    MarkLocation(location, basic.date_time_utc);

    const WaypointFactory factory(WaypointOrigin::USER, 0,
                                  data_components->terrain.get());
    Waypoint wp = factory.Create(location);
    factory.FallbackElevation(wp);

    char name[64] = "Marker";
    if (basic.date_time_utc.IsPlausible()) {
      auto *p = name + StringLength(name);
      *p++ = ' ' ;
      FormatISO8601(p, basic.date_time_utc);
    }

    wp.name = name;
    wp.type = Waypoint::Type::MARKER;

    SuspendAppendSaveWaypoint(std::move(wp));

    if (CommonInterface::GetUISettings().sound.sound_modes_enabled)
      PlayResource("IDR_WAV_CLEAR");
  }

  trigger_redraw();
}

void
InputEvents::eventPilotEvent([[maybe_unused]] const char *misc)
try {
  // Configure start window
  const OrderedTaskSettings &ots =
    backend_components->protected_task_manager->GetOrderedTaskSettings();
  const StartConstraints &constraints = ots.start_constraints;

  const auto now = CommonInterface::Basic().time;

  // Note: pev_start_wait_time == 0 means window starts right away
  TimeStamp new_start_ts( now.ToDuration() + constraints.pev_start_wait_time );
  FineTime new_start(new_start_ts);

  FineTime new_end = (constraints.pev_start_window.count() > 0) ?
                     FineTime(new_start_ts + constraints.pev_start_window) :
                     FineTime::Invalid();

  const TimeSpan ts = TimeSpan(new_start, new_end);

  backend_components->protected_task_manager->SetPevStartTimeSpan(ts);

  // Log pilot event
  if (backend_components->igc_logger)
    backend_components->igc_logger->LogPilotEvent(CommonInterface::Basic());

  // Let devices know the pilot event was pressed
  if (backend_components->devices) {
    MessageOperationEnvironment env;
    backend_components->devices->PutPilotEvent(env);
  }
} catch (...) {
  ShowError(std::current_exception(), _("Logger Error"));
}

void
InputEvents::eventScreenModes(const char *misc)
{
  // toggle switches like this:
  //  -- normal infobox
  //  -- auxiliary infobox
  //  -- full screen
  //  -- normal infobox

  const UIState &ui_state = CommonInterface::GetUIState();

  if (StringIsEqual(misc, "normal")) {
    PageActions::OpenLayout(PageLayout::Default());
  } else if (StringIsEqual(misc, "auxilary")) {
    PageActions::OpenLayout(PageLayout::Aux());
  } else if (StringIsEqual(misc, "toggleauxiliary")) {
    PageActions::OpenLayout(ui_state.auxiliary_enabled
                            ? PageLayout::Default()
                            : PageLayout::Aux());
  } else if (StringIsEqual(misc, "full")) {
    PageActions::OpenLayout(PageLayout::FullScreen());
  } else if (StringIsEqual(misc, "togglefull")) {
    CommonInterface::main_window->SetFullScreen(
        !CommonInterface::main_window->GetFullScreen());
  } else if (StringIsEqual(misc, "show")) {
    if (CommonInterface::main_window->GetFullScreen())
      Message::AddMessage(_("Screen Mode Full"));
    else if (ui_state.auxiliary_enabled)
        Message::AddMessage(_("Auxiliary InfoBoxes"));
    else
        Message::AddMessage(_("Default InfoBoxes"));
  } else if (StringIsEqual(misc, "previous"))
    PageActions::Prev();
  else
    PageActions::Next();

  TriggerMapUpdate();
}

// ClearStatusMessages
// Do Clear Event Warnings
void
InputEvents::eventClearStatusMessages([[maybe_unused]] const char *misc)
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
InputEvents::eventMode(const char *misc)
{
  assert(misc != NULL);

  CommonInterface::main_window->SetDefaultFocus();

  InputEvents::setMode(misc);
}

// Don't think we need this.
void
InputEvents::eventMainMenu([[maybe_unused]] const char *misc)
{
  // todo: popup main menu
}

// Checklist
// Displays the checklist dialog
//  See the checklist dialog section of the reference manual for more info.
void
InputEvents::eventChecklist([[maybe_unused]] const char *misc)
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
InputEvents::eventStatus(const char *misc)
{
  if (StringIsEqual(misc, "system")) {
    dlgStatusShowModal(1);
  } else if (StringIsEqual(misc, "task")) {
    dlgStatusShowModal(2);
  } else if (StringIsEqual(misc, "aircraft")) {
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
InputEvents::eventAnalysis([[maybe_unused]] const char *misc)
{
  dlgAnalysisShowModal(*CommonInterface::main_window,
                       CommonInterface::main_window->GetLook(),
                       CommonInterface::Full(),
                       *backend_components->glide_computer,
                       data_components->airspaces.get(),
                       data_components->terrain.get());
}

// WaypointDetails
// Displays waypoint details
//         current: the current active waypoint
//          select: brings up the waypoint selector, if the user then
//                  selects a waypoint, then the details dialog is shown.
//  See the waypoint dialog section of the reference manual
// for more info.
void
InputEvents::eventWaypointDetails(const char *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  WaypointPtr wp;

  bool allow_navigation = true;
  bool allow_edit = true;

  if (StringIsEqual(misc, "current")) {
    if (!backend_components->protected_task_manager)
      return;

    wp = backend_components->protected_task_manager->GetActiveWaypoint();
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
  } else if (StringIsEqual(misc, "select")) {
    wp = ShowWaypointListDialog(*data_components->waypoints, basic.location);
  }
  if (wp)
    dlgWaypointDetailsShowModal(data_components->waypoints.get(),
                                std::move(wp),
                                allow_navigation, allow_edit);
}

void
InputEvents::eventWaypointEditor([[maybe_unused]] const char *misc)
{
  dlgConfigWaypointsShowModal(*data_components->waypoints);
}

// StatusMessage
// Displays a user defined status message.
//    The argument is the text to be displayed.
//    No punctuation characters are allowed.
void
InputEvents::eventStatusMessage(const char *misc)
{
  if (misc != NULL)
    Message::AddMessage(gettext(misc));
}

// Plays a sound from the filename
void
InputEvents::eventPlaySound(const char *misc)
{
  PlayResource(misc);
}

void
InputEvents::eventAutoLogger(const char *misc)
{
  if (is_simulator())
    return;

  LoggerSettings::AutoLogger auto_logger =
    CommonInterface::GetComputerSettings().logger.auto_logger;

  if (auto_logger == LoggerSettings::AutoLogger::OFF)
    return;

  if (auto_logger == LoggerSettings::AutoLogger::START_ONLY &&
      !StringIsEqual(misc, "start"))
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
InputEvents::eventLogger(const char *misc)
try {
  auto *logger = backend_components->igc_logger.get();
  if (logger == nullptr)
    return;

  // TODO feature: start logger without requiring feedback
  // start stop toggle addnote

  const NMEAInfo &basic = CommonInterface::Basic();
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  if (StringIsEqual(misc, "start ask"))
    logger->GUIStartLogger(basic, settings_computer,
                           backend_components->protected_task_manager.get());
  else if (StringIsEqual(misc, "start"))
    logger->GUIStartLogger(basic, settings_computer,
                           backend_components->protected_task_manager.get(),
                           true);
  else if (StringIsEqual(misc, "stop ask"))
    logger->GUIStopLogger(basic);
  else if (StringIsEqual(misc, "stop"))
    logger->GUIStopLogger(basic, true);
  else if (StringIsEqual(misc, "toggle ask"))
    logger->GUIToggleLogger(basic, settings_computer,
                            backend_components->protected_task_manager.get());
  else if (StringIsEqual(misc, "toggle"))
    logger->GUIToggleLogger(basic, settings_computer,
                            backend_components->protected_task_manager.get(),
                            true);
  else if (StringIsEqual(misc, "nmea")) {
    backend_components->nmea_logger->ToggleEnabled();
    if (backend_components->nmea_logger->IsEnabled()) {
      Message::AddMessage(_("NMEA log on"));
    } else {
      Message::AddMessage(_("NMEA log off"));
    }
  } else if (StringIsEqual(misc, "show"))
    if (logger->IsLoggerActive()) {
      Message::AddMessage(_("Logger on"));
    } else {
      Message::AddMessage(_("Logger off"));
    }
  else if (StringIsEqual(misc, "note", 4))
    // add note to logger file if available..
    logger->LoggerNote(misc + 4);
} catch (...) {
  ShowError(std::current_exception(), _("Logger Error"));
}

// RepeatStatusMessage
// Repeats the last status message.  If pressed repeatedly, will
// repeat previous status messages
void
InputEvents::eventRepeatStatusMessage([[maybe_unused]] const char *misc)
{
  // new interface
  // TODO enhancement: display only by type specified in misc field
  if (CommonInterface::main_window->popup != nullptr)
    CommonInterface::main_window->popup->Repeat();
}

// QuickGuide
// Open the Quick Guide dialog
void
InputEvents::eventQuickGuide([[maybe_unused]] const char *misc)
{
  dlgQuickGuideShowModal(true);
}

// GestureHelp
// Open the standalone gesture help dialog
void
InputEvents::eventGestureHelp([[maybe_unused]] const char *misc)
{
  dlgGestureHelpShowModal();
}

// NearestWaypointDetails
// Displays the waypoint details dialog
void
InputEvents::eventNearestWaypointDetails([[maybe_unused]] const char *misc)
{
  const auto location = GetVisibleLocation();
  if (!location.IsValid())
    return;

  // big range..
  PopupNearestWaypointDetails(*data_components->waypoints, location, 1.0e5);
}

// NearestMapItems
// Displays the map item list dialog
void
InputEvents::eventNearestMapItems([[maybe_unused]] const char *misc)
{
  const auto location = GetVisibleLocation();
  if (!location.IsValid())
    return;

  CommonInterface::main_window->GetMap()->ShowMapItems(location, true, false);
}

// Null
// The null event does nothing.  This can be used to override
// default functionality
void
InputEvents::eventNull([[maybe_unused]] const char *misc)
{
  // do nothing
}

void
InputEvents::eventBeep([[maybe_unused]] const char *misc)
{
#ifdef _WIN32
  MessageBeep(MB_ICONEXCLAMATION);
#else
  PlayResource("IDR_WAV_CLEAR");
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
InputEvents::eventSetup(const char *misc)
{
  if (StringIsEqual(misc, "Basic"))
    dlgBasicSettingsShowModal();
  else if (StringIsEqual(misc, "Wind"))
    ShowWindSettingsDialog();
  else if (StringIsEqual(misc, "System"))
    SystemConfiguration();
  else if (StringIsEqual(misc, "Task"))
    dlgTaskManagerShowModal();
  else if (StringIsEqual(misc, "Airspace"))
    dlgAirspaceShowModal(false);
  else if (StringIsEqual(misc, "Weather"))
    ShowWeatherDialog("rasp");
  else if (StringIsEqual(misc, "Replay")) {
    if (!CommonInterface::MovementDetected() && backend_components->replay)
      ShowReplayDialog(*backend_components->replay);
  } else if (StringIsEqual(misc, "Switches"))
    dlgSwitchesShowModal();
  else if (StringIsEqual(misc, "Teamcode"))
    dlgTeamCodeShowModal();
  else if (StringIsEqual(misc, "Target"))
    dlgTargetShowModal();
  else if (StringIsEqual(misc, "Plane"))
    dlgPlanesShowModal();
  else if (StringIsEqual(misc, "Profile"))
    ProfileListDialog();
  else if (StringIsEqual(misc, "Alternates"))
    dlgAlternatesListShowModal(data_components->waypoints.get());

  trigger_redraw();
}

void
InputEvents::eventCredits([[maybe_unused]] const char *misc)
{
  dlgCreditsShowModal(*CommonInterface::main_window);
}

// Run
// Runs an external program of the specified filename.
// Note that XCSoar will wait until this program exits.
void
InputEvents::eventRun(const char *misc)
{
#ifdef _WIN32
  PROCESS_INFORMATION pi;
  if (!::CreateProcess(misc, NULL, NULL, NULL, false, 0, NULL, NULL, NULL, &pi))
    return;

  // wait for program to finish!
  ::WaitForSingleObject(pi.hProcess, INFINITE);
  ::CloseHandle(pi.hProcess);
  ::CloseHandle(pi.hThread);

#elif defined(__APPLE__) && TARGET_OS_IPHONE
  // can't execute external programs on iOS
  (void)misc;
#else
  if (system(misc)) {;} // Ignore return value
#endif
}

void
InputEvents::eventBrightness([[maybe_unused]] const char *misc)
{
  // not implemented (was only implemented on Altair)
}

void
InputEvents::eventExit([[maybe_unused]] const char *misc)
{
  UIActions::SignalShutdown(false);
}

void
InputEvents::eventUserDisplayModeForce(const char *misc)
{
  UIState &ui_state = CommonInterface::SetUIState();

  if (StringIsEqual(misc, "unforce"))
    ui_state.force_display_mode = DisplayMode::NONE;
  else if (StringIsEqual(misc, "forceclimb"))
    ui_state.force_display_mode = DisplayMode::CIRCLING;
  else if (StringIsEqual(misc, "forcecruise"))
    ui_state.force_display_mode = DisplayMode::CRUISE;
  else if (StringIsEqual(misc, "forcefinal"))
    ui_state.force_display_mode = DisplayMode::FINAL_GLIDE;

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState();
}

void
InputEvents::eventAddWaypoint(const char *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  auto &way_points = *data_components->waypoints;

  if (StringIsEqual(misc, "goto")) {
    const auto location = GetVisibleLocation();
    if (!location.IsValid())
      return;

    double elevation = std::numeric_limits<double>::quiet_NaN();
    if (data_components->terrain != nullptr) {
      const auto h = data_components->terrain->GetTerrainHeight(location);
      if (!h.IsSpecial()) {
        elevation = h.GetValue();
      }
    }

    ScopeSuspendAllThreads suspend;
    way_points.AddTempPoint(location, elevation, "(goto)");
  } else {
    Waypoint edit_waypoint = way_points.Create(basic.location);
    edit_waypoint.elevation = calculated.terrain_altitude;
    if (dlgWaypointEditShowModal(edit_waypoint) == WaypointEditResult::CANCEL ||
        edit_waypoint.name.empty()) {
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

eventTaskLoad         - Load tasks from a file (misc = filename)
eventTaskSave         - Save tasks to a file (misc = filename)
eventProfileLoad      - Load profile from a file (misc = filename)
eventProfileSave      - Save profile to a file (misc = filename)

*/

/* TODO feature: - new events

eventPanWaypoint                    - Set pan to a waypoint
- Waypoint could be "next", "first", "last", "previous", or named
- Note: wrong name - probably just part of eventPan
eventPressure         - Increase, Decrease, show, Set pressure value
eventDeclare          - (JMW separate from internal logger)
eventAirspaceDisplay  - all, below nnn, below me, auto nnn
eventAirspaceWarnings - on, off, time nn, ack nn
eventTerrain          - see map_window.Event_Terrain
eventCompass          - on, off, cruise on, crusie off, climb on, climb off
eventVario            - on, off // JMW what does this do?
eventTerrainRange     - on, off (might be part of eventTerrain)
eventSounds           - Include Task and Modes sounds along with Vario
                      - Include master nn, deadband nn, netto trigger
mph/kts/...

*/

// helpers

void
InputEvents::eventWeather(const char *misc)
{
  ShowWeatherDialog(misc);
}

void
InputEvents::eventQuickMenu([[maybe_unused]] const char *misc)
{
 dlgQuickMenuShowModal(*CommonInterface::main_window);
}

void
InputEvents::eventFileManager([[maybe_unused]] const char *misc)
{
  ShowFileManager();
}

void
InputEvents::eventDataManagement([[maybe_unused]] const char *misc)
{
  ShowDataManagementDialog();
}

void
InputEvents::eventExportFlights([[maybe_unused]] const char *misc)
{
  ShowExportFlightsDialog();
}

void
InputEvents::eventExchangeFrequencies([[maybe_unused]] const char *misc)
{
  XCSoarInterface::ExchangeRadioFrequencies(true);
}

void
InputEvents::eventUploadIGCFile([[maybe_unused]] const char *misc) {
  FileDataField df;
  df.ScanMultiplePatterns("*.igc\0");
  df.SetFileType(FileType::IGC);
  if (FilePicker("IGC-FilePicker", df)) {
    auto path = df.GetValue();
    if (!path.empty())
      if (WeGlide::UploadIGCFile(path)) {
        // success!
      }
  }
}
