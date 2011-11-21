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
#include "LogFile.hpp"
#include "Device/Parser.hpp"
#include "UIState.hpp"
#include "SettingsComputer.hpp"
#include "SettingsMap.hpp"
#include "Math/FastMath.h"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Airspace.hpp"
#include "Dialogs/Task.hpp"
#include "Dialogs/Traffic.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Dialogs/Weather.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/Planes.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "Message.hpp"
#include "ProtectedMarkers.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "MainWindow.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "Audio/Sound.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Language/Language.hpp"
#include "Logger/Logger.hpp"
#include "Asset.hpp"
#include "Logger/NMEALogger.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "DeviceBlackboard.hpp"
#include "UtilsSettings.hpp"
#include "Pages.hpp"
#include "Hardware/AltairControl.hpp"
#include "NMEA/Aircraft.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "Compiler.h"
#include "Net/Features.hpp"

#include <assert.h>
#include <ctype.h>
#include <tchar.h>
#include <algorithm>

static void
trigger_redraw()
{
  if (!XCSoarInterface::Basic().location_available)
    TriggerGPSUpdate();
  TriggerMapUpdate();
}

// -----------------------------------------------------------------------
// Execution - list of things you can do
// -----------------------------------------------------------------------


// TODO code: Keep marker text for use in log file etc.
void
InputEvents::eventMarkLocation(const TCHAR *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (_tcscmp(misc, _T("reset")) == 0) {
    protected_marks->Reset();
  } else {
    protected_marks->MarkLocation(basic.location, basic.date_time_utc);

    if (XCSoarInterface::SettingsComputer().sound_modes_enabled)
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

  using namespace Pages;
  typedef PageSettings::InfoBoxConfig InfoBoxConfig;
  typedef PageSettings::PageLayout PageLayout;

  const UIState &ui_state = CommonInterface::GetUIState();

  if (_tcscmp(misc, _T("normal")) == 0) {
    const PageSettings::PageLayout pl(PageLayout::tlMapAndInfoBoxes,
                                      InfoBoxConfig(true, 0));
    OpenLayout(pl);
  } else if (_tcscmp(misc, _T("auxilary")) == 0) {
    const PageSettings::PageLayout pl(PageLayout::tlMapAndInfoBoxes,
                                      InfoBoxConfig(false, 3));
    OpenLayout(pl);
  } else if (_tcscmp(misc, _T("toggleauxiliary")) == 0) {
    const PageLayout pl(!ui_state.auxiliary_enabled ?
                        PageLayout(PageLayout::tlMapAndInfoBoxes,
                                   PageSettings::InfoBoxConfig(false, 3)) :
                        PageLayout(PageLayout::tlMapAndInfoBoxes,
                                   PageSettings::InfoBoxConfig(true, 0)));
    OpenLayout(pl);
  } else if (_tcscmp(misc, _T("full")) == 0) {
    const PageLayout pl(PageLayout::tlMap,
                        PageSettings::InfoBoxConfig(true, 0));
    OpenLayout(pl);
  } else if (_tcscmp(misc, _T("togglefull")) == 0) {
    XCSoarInterface::main_window.SetFullScreen(
        !XCSoarInterface::main_window.GetFullScreen());
  } else if (_tcscmp(misc, _T("show")) == 0) {
    if (XCSoarInterface::main_window.GetFullScreen())
      Message::AddMessage(_("Screen Mode Full"));
    else if (ui_state.auxiliary_enabled)
        Message::AddMessage(_("Auxiliary InfoBoxes"));
    else
        Message::AddMessage(_("Default InfoBoxes"));
  } else if (_tcscmp(misc, _T("previous")) == 0)
    Pages::Prev();
  else
    Pages::Next();


  trigger_redraw();
}

// Do clear warnings IF NONE Toggle Terrain/Topography
void
InputEvents::eventClearWarningsOrTerrainTopology(gcc_unused const TCHAR *misc)
{
  if (airspace_warnings != NULL && !airspace_warnings->warning_empty()) {
    airspace_warnings->clear_warnings();
    return;
  }
  // Else toggle TerrainTopography - and show the results
  sub_TerrainTopography(-1);
  sub_TerrainTopography(0);
  XCSoarInterface::SendSettingsMap(true);
}

// ClearStatusMessages
// Do Clear Event Warnings
void
InputEvents::eventClearStatusMessages(gcc_unused const TCHAR *misc)
{
  // TODO enhancement: allow selection of specific messages (here we are acknowledging all)
  XCSoarInterface::main_window.popup.Acknowledge(0);
}

void
InputEvents::eventFLARMRadar(gcc_unused const TCHAR *misc)
{
  if (_tcscmp(misc, _T("ForceToggle")) == 0) {
    CommonInterface::main_window.ToggleForceFLARMRadar();
  } else
    CommonInterface::main_window.ToggleSuppressFLARMRadar();
}

void
InputEvents::eventThermalAssistant(gcc_unused const TCHAR *misc)
{
  dlgThermalAssistantShowModal();
}

// Mode
// Sets the current event mode.
//  The argument is the label of the mode to activate.
//  This is used to activate menus/submenus of buttons
void
InputEvents::eventMode(const TCHAR *misc)
{
  assert(misc != NULL);

  XCSoarInterface::main_window.SetDefaultFocus();

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

// FLARM Traffic
// Displays the FLARM traffic dialog
//  See the checklist dialog section of the reference manual for more info.
void
InputEvents::eventFlarmTraffic(gcc_unused const TCHAR *misc)
{
  if (XCSoarInterface::Basic().flarm.available)
    dlgFlarmTrafficShowModal();
}

void
InputEvents::eventFlarmDetails(gcc_unused const TCHAR *misc)
{
  StaticString<4> callsign;
  callsign.clear();
  if (!TextEntryDialog(CommonInterface::main_window, callsign,
                       _("Competition ID")))
    return;

  const FlarmId *ids[30];
  unsigned count = FlarmDetails::FindIdsByCallSign(callsign, ids, 30);

  if (count > 0) {
    const FlarmId *id = dlgFlarmDetailsListShowModal(
        XCSoarInterface::main_window, _("Show details:"), ids, count);

    if (id != NULL && id->IsDefined())
      dlgFlarmTrafficDetailsShowModal(*id);
  } else {
    MessageBoxX(_("Unknown competition number"),
                _("Not found"), MB_OK | MB_ICONINFORMATION);
  }
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
  if (_tcscmp(misc, _T("system")) == 0) {
    dlgStatusShowModal(1);
  } else if (_tcscmp(misc, _T("task")) == 0) {
    dlgStatusShowModal(2);
  } else if (_tcscmp(misc, _T("aircraft")) == 0) {
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
  dlgAnalysisShowModal(XCSoarInterface::main_window,
                       CommonInterface::main_window.GetLook(),
                       CommonInterface::Full(),
                       *glide_computer,
                       protected_task_manager, &airspace_database, terrain);
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
  const Waypoint* wp = NULL;

  if (_tcscmp(misc, _T("current")) == 0) {
    if (protected_task_manager == NULL)
      return;

    wp = protected_task_manager->GetActiveWaypoint();
    if (!wp) {
      Message::AddMessage(_("No active waypoint!"));
      return;
    }
  } else if (_tcscmp(misc, _T("select")) == 0) {
    wp = dlgWaypointSelect(XCSoarInterface::main_window, basic.location);
  }
  if (wp)
    dlgWaypointDetailsShowModal(XCSoarInterface::main_window, *wp);
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
  if (!XCSoarInterface::SettingsComputer().auto_logger_disabled)
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
  if (protected_task_manager == NULL)
    return;

  // TODO feature: start logger without requiring feedback
  // start stop toggle addnote

  const NMEAInfo &basic = CommonInterface::Basic();
  const SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SettingsComputer();

  if (_tcscmp(misc, _T("start ask")) == 0)
    logger.GUIStartLogger(basic, settings_computer,
                          *protected_task_manager);
  else if (_tcscmp(misc, _T("start")) == 0)
    logger.GUIStartLogger(basic, settings_computer,
                          *protected_task_manager, true);
  else if (_tcscmp(misc, _T("stop ask")) == 0)
    logger.GUIStopLogger(basic);
  else if (_tcscmp(misc, _T("stop")) == 0)
    logger.GUIStopLogger(basic, true);
  else if (_tcscmp(misc, _T("toggle ask")) == 0)
    logger.GUIToggleLogger(basic, settings_computer,
                           *protected_task_manager);
  else if (_tcscmp(misc, _T("toggle")) == 0)
    logger.GUIToggleLogger(basic, settings_computer,
                           *protected_task_manager, true);
  else if (_tcscmp(misc, _T("nmea")) == 0) {
    NMEALogger::enabled = !NMEALogger::enabled;
    if (NMEALogger::enabled) {
      Message::AddMessage(_("NMEA log on"));
    } else {
      Message::AddMessage(_("NMEA log off"));
    }
  } else if (_tcscmp(misc, _T("show")) == 0)
    if (logger.IsLoggerActive()) {
      Message::AddMessage(_("Logger on"));
    } else {
      Message::AddMessage(_("Logger off"));
    }
  else if (_tcsncmp(misc, _T("note"), 4) == 0)
    // add note to logger file if available..
    logger.LoggerNote(misc + 4);
}

// RepeatStatusMessage
// Repeats the last status message.  If pressed repeatedly, will
// repeat previous status messages
void
InputEvents::eventRepeatStatusMessage(gcc_unused const TCHAR *misc)
{
  // new interface
  // TODO enhancement: display only by type specified in misc field
  XCSoarInterface::main_window.popup.Repeat(0);
}

// NearestWaypointDetails
// Displays the waypoint details dialog
//  aircraft: the waypoint nearest the aircraft
//  pan: the waypoint nearest to the pan cursor
void
InputEvents::eventNearestWaypointDetails(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("aircraft")) == 0)
    // big range..
    PopupNearestWaypointDetails(way_points, CommonInterface::Basic().location,
                                1.0e5);
  else if (_tcscmp(misc, _T("pan")) == 0)
    // big range..
    PopupNearestWaypointDetails(way_points,
                                CommonInterface::main_window.GetProjection().GetGeoLocation(),
                                1.0e5);
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
  #if defined(GNAV)
  altair_control.ShortBeep();
#elif defined(WIN32)
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
  if (_tcscmp(misc, _T("Basic")) == 0)
    dlgBasicSettingsShowModal();
  else if (_tcscmp(misc, _T("Wind")) == 0)
    dlgWindSettingsShowModal();
  else if (_tcscmp(misc, _T("System")) == 0)
    SystemConfiguration();
  else if (_tcscmp(misc, _T("Task")) == 0)
    dlgTaskManagerShowModal(XCSoarInterface::main_window);
  else if (_tcscmp(misc, _T("Airspace")) == 0)
    dlgAirspaceShowModal(false);
  else if (_tcscmp(misc, _T("Weather")) == 0)
    dlgWeatherShowModal();
  else if (_tcscmp(misc, _T("Replay")) == 0) {
    if (!CommonInterface::MovementDetected())
      dlgLoggerReplayShowModal();
  } else if (_tcscmp(misc, _T("Switches")) == 0)
    dlgSwitchesShowModal();
  else if (_tcscmp(misc, _T("Voice")) == 0)
    dlgVoiceShowModal();
  else if (_tcscmp(misc, _T("Teamcode")) == 0)
    dlgTeamCodeShowModal();
  else if (_tcscmp(misc, _T("Target")) == 0)
    dlgTargetShowModal();
  else if (_tcscmp(misc, _T("Plane")) == 0)
    dlgPlanesShowModal(XCSoarInterface::main_window);
  else if (_tcscmp(misc, _T("Alternates")) == 0)
    dlgAlternatesListShowModal(XCSoarInterface::main_window);

  trigger_redraw();
}

void
InputEvents::eventCredits(gcc_unused const TCHAR *misc)
{
  dlgCreditsShowModal(XCSoarInterface::main_window);
}

// Run
// Runs an external program of the specified filename.
// Note that XCSoar will wait until this program exits.
void
InputEvents::eventRun(const TCHAR *misc)
{
  #ifdef WIN32
  PROCESS_INFORMATION pi;
  if (!::CreateProcess(misc, NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi))
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
  dlgBrightnessShowModal();
}

void
InputEvents::eventExit(gcc_unused const TCHAR *misc)
{
  XCSoarInterface::SignalShutdown(false);
}

void
InputEvents::eventUserDisplayModeForce(const TCHAR *misc)
{
  UIState &ui_state = CommonInterface::SetUIState();

  if (_tcscmp(misc, _T("unforce")) == 0)
    ui_state.force_display_mode = DM_NONE;
  else if (_tcscmp(misc, _T("forceclimb")) == 0)
    ui_state.force_display_mode = DM_CIRCLING;
  else if (_tcscmp(misc, _T("forcecruise")) == 0)
    ui_state.force_display_mode = DM_CRUISE;
  else if (_tcscmp(misc, _T("forcefinal")) == 0)
    ui_state.force_display_mode = DM_FINAL_GLIDE;
  else if (_tcscmp(misc, _T("show")) == 0)
    Message::AddMessage(_("Map labels on"));

  /* trigger mode update by GlueMapWindow */
  CommonInterface::main_window.full_redraw();
}

void
InputEvents::eventAddWaypoint(const TCHAR *misc)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (_tcscmp(misc, _T("takeoff")) == 0) {
    if (basic.location_available && calculated.terrain_valid) {
      ScopeSuspendAllThreads suspend;
      way_points.add_takeoff_point(basic.location, calculated.terrain_altitude);
      way_points.optimise();
    }
  } else {
    Waypoint edit_waypoint = way_points.create(basic.location);
    edit_waypoint.altitude = calculated.terrain_altitude;
    if (!dlgWaypointEditShowModal(edit_waypoint) || edit_waypoint.name.empty()) {
      trigger_redraw();
      return;
    }
    {
      ScopeSuspendAllThreads suspend;
      way_points.append(edit_waypoint);
      way_points.optimise();
    }
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
#ifdef HAVE_NET
  if (_tcscmp(misc, _T("list")) == 0)
    dlgNOAAListShowModal(XCSoarInterface::main_window);
#endif
}
