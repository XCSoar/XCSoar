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
#include "Device/device.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
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
#include "Dialogs/AirspaceWarningDialog.hpp"
#include "Message.hpp"
#include "Marks.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Units/UnitsFormatter.hpp"
#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Atmosphere/CuSonde.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Profile/Profile.hpp"
#include "LocalPath.hpp"
#include "Profile/ProfileKeys.hpp"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "Audio/Sound.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Language/Language.hpp"
#include "Logger/Logger.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Asset.hpp"
#include "Logger/NMEALogger.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceAircraftPerformance.hpp"
#include "DeviceBlackboard.hpp"
#include "UtilsSettings.hpp"
#include "Pages.hpp"
#include "Hardware/AltairControl.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AirspaceSoonestSort.hpp"
#include "LocalTime.hpp"
#include "NMEA/Aircraft.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "Compiler.h"

#include <assert.h>
#include <ctype.h>
#include <tchar.h>
#include <algorithm>

using std::min;
using std::max;

static void
trigger_redraw()
{
  if (!XCSoarInterface::Basic().LocationAvailable)
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
  const NMEA_INFO &basic = CommonInterface::Basic();

  if (_tcscmp(misc, _T("reset")) == 0) {
    marks->Reset();
  } else {
    marks->MarkLocation(basic.Location, basic.DateTime);

    if (XCSoarInterface::SettingsComputer().EnableSoundModes)
      PlayResource(_T("IDR_WAV_CLEAR"));
  }

  trigger_redraw();
}

void
InputEvents::eventSounds(const TCHAR *misc)
{
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();
 // bool OldEnableSoundVario = EnableSoundVario;

  if (_tcscmp(misc, _T("toggle")) == 0)
    settings_computer.EnableSoundVario = !settings_computer.EnableSoundVario;
  else if (_tcscmp(misc, _T("on")) == 0)
    settings_computer.EnableSoundVario = true;
  else if (_tcscmp(misc, _T("off")) == 0)
    settings_computer.EnableSoundVario = false;
  else if (_tcscmp(misc, _T("show")) == 0) {
    if (settings_computer.EnableSoundVario)
      Message::AddMessage(_("Vario sounds on"));
    else
      Message::AddMessage(_("Vario sounds off"));
  }
  /*
  if (EnableSoundVario != OldEnableSoundVario) {
    VarioSound_EnableSound(EnableSoundVario);
  }
  */
}

void
InputEvents::eventSnailTrail(const TCHAR *misc)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (_tcscmp(misc, _T("toggle")) == 0) {
    unsigned trail_length = (int)settings_map.trail_length;
    trail_length = (trail_length + 1u) % 4u;
    settings_map.trail_length = (TrailLength)trail_length;
  } else if (_tcscmp(misc, _T("off")) == 0)
    settings_map.trail_length = TRAIL_OFF;
  else if (_tcscmp(misc, _T("long")) == 0)
    settings_map.trail_length = TRAIL_LONG;
  else if (_tcscmp(misc, _T("short")) == 0)
    settings_map.trail_length = TRAIL_SHORT;
  else if (_tcscmp(misc, _T("full")) == 0)
    settings_map.trail_length = TRAIL_FULL;
  else if (_tcscmp(misc, _T("show")) == 0) {
    switch (settings_map.trail_length) {
    case TRAIL_OFF:
      Message::AddMessage(_("Snail trail off"));
      break;

    case TRAIL_LONG:
      Message::AddMessage(_("Long snail trail"));
      break;

    case TRAIL_SHORT:
      Message::AddMessage(_("Short snail trail"));
      break;

    case TRAIL_FULL:
      Message::AddMessage(_("Full snail trail"));
      break;
    }
  }

  ActionInterface::SendSettingsMap(true);
}

// VENTA3
/*
 * This even currently toggles DrawAirSpace() and does nothing else.
 * But since we use an int and not a bool, it is easy to expand it.
 * Note that XCSoar.cpp init OnAirSpace always to 1, and this value
 * is never saved to the registry actually. It is intended to be used
 * as a temporary choice during flight, does not affect configuration.
 * Note also that in MapWindow DrawAirSpace() is accomplished for
 * every OnAirSpace value >0 .  We can use negative numbers also,
 * but 0 should mean OFF all the way.
 */
void
InputEvents::eventAirSpace(const TCHAR *misc)
{
  AirspaceRendererSettings &settings =
    CommonInterface::SetSettingsMap().airspace;

  if (_tcscmp(misc, _T("toggle")) == 0)
    settings.enable = !settings.enable;
  else if (_tcscmp(misc, _T("off")) == 0)
    settings.enable = false;
  else if (_tcscmp(misc, _T("on")) == 0)
    settings.enable = true;
  else if (_tcscmp(misc, _T("show")) == 0) {
    if (!settings.enable)
      Message::AddMessage(_("Show airspace off"));
    if (settings.enable)
      Message::AddMessage(_("Show airspace on"));
  }

  ActionInterface::SendSettingsMap(true);
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

  if (_tcscmp(misc, _T("normal")) == 0) {
    PageLayout pl(PageLayout::tlMapAndInfoBoxes);
    OpenLayout(pl);
  } else if (_tcscmp(misc, _T("auxilary")) == 0) {
    PageLayout pl(PageLayout::tlMapAndInfoBoxes, InfoBoxConfig(false, 3));
    OpenLayout(pl);
  } else if (_tcscmp(misc, _T("toggleauxiliary")) == 0) {
    PageLayout pl(!XCSoarInterface::SettingsMap().EnableAuxiliaryInfo ?
      PageLayout(PageLayout::tlMapAndInfoBoxes, InfoBoxConfig(false, 3)) :
      PageLayout(PageLayout::tlMapAndInfoBoxes));
    OpenLayout(pl);
  } else if (_tcscmp(misc, _T("full")) == 0) {
    PageLayout pl(PageLayout::tlMap);
    OpenLayout(pl);
  } else if (_tcscmp(misc, _T("togglefull")) == 0) {
    XCSoarInterface::main_window.SetFullScreen(
        !XCSoarInterface::main_window.GetFullScreen());
  } else if (_tcscmp(misc, _T("show")) == 0) {
    if (XCSoarInterface::main_window.GetFullScreen())
      Message::AddMessage(_("Screen Mode Full"));
    else if (XCSoarInterface::SettingsMap().EnableAuxiliaryInfo)
        Message::AddMessage(_("Auxiliary InfoBoxes"));
    else
        Message::AddMessage(_("Default InfoBoxes"));
  } else if (_tcscmp(misc, _T("togglebiginfo")) == 0) {
    InfoBoxLayout::fullscreen = !InfoBoxLayout::fullscreen;
    InfoBoxManager::SetDirty();
  } else if (_tcscmp(misc, _T("previous")) == 0)
    Pages::Prev();
  else
    Pages::Next();


  trigger_redraw();
}

// eventAutoZoom - Turn on|off|toggle AutoZoom
// misc:
//	auto on - Turn on if not already
//	auto off - Turn off if not already
//	auto toggle - Toggle current full screen status
//	auto show - Shows autozoom status
//	+	- Zoom in
//	++	- Zoom in near
//	-	- Zoom out
//	--	- Zoom out far
//	n.n	- Zoom to a set scale
//	show - Show current zoom scale
void
InputEvents::eventZoom(const TCHAR* misc)
{
  // JMW pass through to handler in MapWindow
  // here:
  // -1 means toggle
  // 0 means off
  // 1 means on

  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (_tcscmp(misc, _T("auto toggle")) == 0)
    sub_AutoZoom(-1);
  else if (_tcscmp(misc, _T("auto on")) == 0)
    sub_AutoZoom(1);
  else if (_tcscmp(misc, _T("auto off")) == 0)
    sub_AutoZoom(0);
  else if (_tcscmp(misc, _T("auto show")) == 0) {
    if (settings_map.AutoZoom)
      Message::AddMessage(_("Auto. zoom on"));
    else
      Message::AddMessage(_("Auto. zoom off"));
  } else if (_tcscmp(misc, _T("slowout")) == 0)
    sub_ScaleZoom(-1);
  else if (_tcscmp(misc, _T("slowin")) == 0)
    sub_ScaleZoom(1);
  else if (_tcscmp(misc, _T("out")) == 0)
    sub_ScaleZoom(-1);
  else if (_tcscmp(misc, _T("in")) == 0)
    sub_ScaleZoom(1);
  else if (_tcscmp(misc, _T("-")) == 0)
    sub_ScaleZoom(-1);
  else if (_tcscmp(misc, _T("+")) == 0)
    sub_ScaleZoom(1);
  else if (_tcscmp(misc, _T("--")) == 0)
    sub_ScaleZoom(-2);
  else if (_tcscmp(misc, _T("++")) == 0)
    sub_ScaleZoom(2);
  else if (_tcscmp(misc, _T("circlezoom toggle")) == 0) {
    settings_map.CircleZoom = !settings_map.CircleZoom;
  } else if (_tcscmp(misc, _T("circlezoom on")) == 0) {
    settings_map.CircleZoom = true;
  } else if (_tcscmp(misc, _T("circlezoom off")) == 0) {
    settings_map.CircleZoom = false;
  } else if (_tcscmp(misc, _T("circlezoom show")) == 0) {
    if (settings_map.CircleZoom)
      Message::AddMessage(_("Circling zoom on"));
    else
      Message::AddMessage(_("Circling zoom off"));
  } else {
    TCHAR *endptr;
    double zoom = _tcstod(misc, &endptr);
    if (endptr == misc)
      return;

    sub_SetZoom(Units::ToSysDistance(fixed(zoom)));
  }

  XCSoarInterface::SendSettingsMap(true);
}

/**
 * This function handles all "pan" input events
 * @param misc A string describing the desired pan action.
 *  on             Turn pan on
 *  off            Turn pan off
 *  toogle         Toogles pan mode
 *  supertoggle    Toggles pan mode and fullscreen
 *  up             Pan up
 *  down           Pan down
 *  left           Pan left
 *  right          Pan right
 *  @todo feature: n,n Go that direction - +/-
 *  @todo feature: ??? Go to particular point
 *  @todo feature: ??? Go to waypoint (eg: next, named)
 */
void
InputEvents::eventPan(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("toggle")) == 0)
    sub_Pan(-1);

  else if (_tcscmp(misc, _T("supertoggle")) == 0)
    sub_Pan(-2);

  else if (_tcscmp(misc, _T("on")) == 0)
    sub_Pan(1);

  else if (_tcscmp(misc, _T("off")) == 0)
    sub_Pan(0);

  else if (_tcscmp(misc, _T("up")) == 0)
    if (model_is_hp31x())
      // Scroll wheel on the HP31x series should zoom in pan mode
      sub_ScaleZoom(1);
    else
      sub_PanCursor(0, 1);

  else if (_tcscmp(misc, _T("down")) == 0)
    if (model_is_hp31x())
      // Scroll wheel on the HP31x series should zoom in pan mode
      sub_ScaleZoom(-1);
    else
      sub_PanCursor(0, -1);

  else if (_tcscmp(misc, _T("left")) == 0)
    sub_PanCursor(1, 0);

  else if (_tcscmp(misc, _T("right")) == 0)
    sub_PanCursor(-1, 0);

  else if (_tcscmp(misc, _T("show")) == 0) {
    if (CommonInterface::IsPanning())
      Message::AddMessage(_("Pan mode on"));
    else
      Message::AddMessage(_("Pan mode off"));
  }

  XCSoarInterface::SendSettingsMap(true);
}

void
InputEvents::eventTerrainTopology(const TCHAR *misc)
{
  eventTerrainTopography(misc);
}

// Do JUST Terrain/Topography (toggle any, on/off any, show)
void
InputEvents::eventTerrainTopography(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("terrain toggle")) == 0)
    sub_TerrainTopography(-2);
  else if (_tcscmp(misc, _T("topography toggle")) == 0)
    sub_TerrainTopography(-3);
  else if (_tcscmp(misc, _T("topology toggle")) == 0)
    sub_TerrainTopography(-3);
  else if (_tcscmp(misc, _T("terrain on")) == 0)
    sub_TerrainTopography(3);
  else if (_tcscmp(misc, _T("terrain off")) == 0)
    sub_TerrainTopography(4);
  else if (_tcscmp(misc, _T("topography on")) == 0)
    sub_TerrainTopography(1);
  else if (_tcscmp(misc, _T("topography off")) == 0)
    sub_TerrainTopography(2);
  else if (_tcscmp(misc, _T("topology on")) == 0)
    sub_TerrainTopography(1);
  else if (_tcscmp(misc, _T("topology off")) == 0)
    sub_TerrainTopography(2);
  else if (_tcscmp(misc, _T("show")) == 0)
    sub_TerrainTopography(0);
  else if (_tcscmp(misc, _T("toggle")) == 0)
    sub_TerrainTopography(-1);

  XCSoarInterface::SendSettingsMap(true);
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

// ClearAirspaceWarnings
// Clears airspace warnings for the selected airspace
void
InputEvents::eventClearAirspaceWarnings(gcc_unused const TCHAR *misc)
{
  if (airspace_warnings != NULL)
    airspace_warnings->clear_warnings();
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
  GaugeFLARM *gauge_flarm = XCSoarInterface::main_window.flarm;
  if (gauge_flarm == NULL)
    return;

  if (_tcscmp(misc, _T("ForceToggle")) == 0) {
    gauge_flarm->ForceVisible = !gauge_flarm->ForceVisible;
    XCSoarInterface::SetSettingsMap().EnableFLARMGauge =
        gauge_flarm->ForceVisible;
  } else
    gauge_flarm->Suppress = !gauge_flarm->Suppress;
  // the result of this will get triggered by refreshslots
}

void
InputEvents::eventThermalAssistant(gcc_unused const TCHAR *misc)
{
  dlgThermalAssistantShowModal();
}

// SelectInfoBox
// Selects the next or previous infobox
void
InputEvents::eventSelectInfoBox(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("next")) == 0)
    InfoBoxManager::Event_Select(1);
  else if (_tcscmp(misc, _T("previous")) == 0)
    InfoBoxManager::Event_Select(-1);
}

// ChangeInfoBoxType
// Changes the type of the current infobox to the next/previous type
void
InputEvents::eventChangeInfoBoxType(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("next")) == 0)
    InfoBoxManager::Event_Change(1);
  else if (_tcscmp(misc, _T("previous")) == 0)
    InfoBoxManager::Event_Change(-1);
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
    task_manager->get_task_advance().get_advance_state();
  
  if (_tcscmp(misc, _T("on")) == 0) {
    task_manager->get_task_advance().set_armed(true);
  } else if (_tcscmp(misc, _T("off")) == 0) {
    task_manager->get_task_advance().set_armed(false);
  } else if (_tcscmp(misc, _T("toggle")) == 0) {
    task_manager->get_task_advance().toggle_armed();
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

// DoInfoKey
// Performs functions associated with the selected infobox
//    up: triggers the up event
//    etc.
//    Functions associated with the infoboxes are described in the
//    infobox section in the reference guide
void InputEvents::eventDoInfoKey(const TCHAR *misc) {
  if (_tcscmp(misc, _T("up")) == 0)
    InfoBoxManager::ProcessKey(InfoBoxContent::ibkUp);
  else if (_tcscmp(misc, _T("down")) == 0)
    InfoBoxManager::ProcessKey(InfoBoxContent::ibkDown);
  else if (_tcscmp(misc, _T("left")) == 0)
    InfoBoxManager::ProcessKey(InfoBoxContent::ibkLeft);
  else if (_tcscmp(misc, _T("right")) == 0)
    InfoBoxManager::ProcessKey(InfoBoxContent::ibkRight);
  else if (_tcscmp(misc, _T("return")) == 0)
    InfoBoxManager::ProcessKey(InfoBoxContent::ibkEnter);
  else if (_tcscmp(misc, _T("setup")) == 0)
    InfoBoxManager::SetupFocused();
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
  TCHAR callsign[4] = _T("");
  if (!dlgTextEntryShowModal(CommonInterface::main_window, callsign, 4,
                             _("Competition ID")))
    return;

  const FlarmId *ids[30];
  unsigned count = FlarmDetails::FindIdsByCallSign(callsign, ids, 30);

  if (count > 0) {
    const FlarmId *id = dlgFlarmDetailsListShowModal(
        XCSoarInterface::main_window, _("Show details:"), ids, count);

    if (id != NULL && id->defined())
      dlgFlarmTrafficDetailsShowModal(*id);
  } else {
    MessageBoxX(_("Unknown competition number"),
                _("Not found"), MB_OK | MB_ICONINFORMATION);
  }
}

void
InputEvents::eventCalculator(gcc_unused const TCHAR *misc)
{
  dlgTaskManagerShowModal(XCSoarInterface::main_window);
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
                       *CommonInterface::main_window.look,
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
  const NMEA_INFO &basic = CommonInterface::Basic();
  const Waypoint* wp = NULL;

  if (_tcscmp(misc, _T("current")) == 0) {
    if (protected_task_manager == NULL)
      return;

    wp = protected_task_manager->getActiveWaypoint();
    if (!wp) {
      Message::AddMessage(_("No active waypoint!"));
      return;
    }
  } else if (_tcscmp(misc, _T("select")) == 0) {
    wp = dlgWaypointSelect(XCSoarInterface::main_window, basic.Location);
  }
  if (wp)
    dlgWaypointDetailsShowModal(XCSoarInterface::main_window, *wp);
}

void
InputEvents::eventGotoLookup(gcc_unused const TCHAR *misc)
{
  const NMEA_INFO &basic = CommonInterface::Basic();

  if (protected_task_manager == NULL)
    return;

  const Waypoint* wp = dlgWaypointSelect(XCSoarInterface::main_window,
                                         basic.Location);
  if (wp != NULL) {
    protected_task_manager->do_goto(*wp);
    trigger_redraw();
  }
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

// MacCready
// Adjusts MacCready settings
// up, down, auto on, auto off, auto toggle, auto show
void
InputEvents::eventMacCready(const TCHAR *misc)
{
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();

  if (protected_task_manager == NULL)
    return;

  const GlidePolar &polar = settings_computer.glide_polar_task;
  fixed mc = polar.GetMC();

  if (_tcscmp(misc, _T("up")) == 0) {
    mc = std::min(mc + fixed_one / 10, fixed(5));
    ActionInterface::SetMacCready(mc);
  } else if (_tcscmp(misc, _T("down")) == 0) {
    mc = std::max(mc - fixed_one / 10, fixed_zero);
    ActionInterface::SetMacCready(mc);
  } else if (_tcscmp(misc, _T("auto toggle")) == 0) {
    settings_computer.auto_mc = !settings_computer.auto_mc;
    Profile::Set(szProfileAutoMc, settings_computer.auto_mc);
  } else if (_tcscmp(misc, _T("auto on")) == 0) {
    settings_computer.auto_mc = true;
    Profile::Set(szProfileAutoMc, true);
  } else if (_tcscmp(misc, _T("auto off")) == 0) {
    settings_computer.auto_mc = false;
    Profile::Set(szProfileAutoMc, false);
  } else if (_tcscmp(misc, _T("auto show")) == 0) {
    if (settings_computer.auto_mc) {
      Message::AddMessage(_("Auto. MacCready on"));
    } else {
      Message::AddMessage(_("Auto. MacCready off"));
    }
  } else if (_tcscmp(misc, _T("show")) == 0) {
    TCHAR Temp[100];
    Units::FormatUserVSpeed(mc,
                            Temp, sizeof(Temp) / sizeof(Temp[0]),
                            false);
    Message::AddMessage(_("MacCready "), Temp);
  }
}

// SendNMEA
//  Sends a user-defined NMEA string to an external instrument.
//   The string sent is prefixed with the start character '$'
//   and appended with the checksum e.g. '*40'.  The user needs only
//   to provide the text in between the '$' and '*'.
//
void
InputEvents::eventSendNMEA(const TCHAR *misc)
{
  if (misc)
    VarioWriteNMEA(misc);
}

void
InputEvents::eventSendNMEAPort1(const TCHAR *misc)
{
  const unsigned i = 0;

  if (misc != NULL && i < NUMDEV)
    DeviceList[i].WriteNMEA(misc);
}

void
InputEvents::eventSendNMEAPort2(const TCHAR *misc)
{
  const unsigned i = 1;

  if (misc != NULL && i < NUMDEV)
    DeviceList[i].WriteNMEA(misc);
}

// AdjustVarioFilter
// When connected to the Vega variometer, this adjusts
// the filter time constant
//     slow/medium/fast
// The following arguments can be used for diagnostics purposes
//     statistics:
//     diagnostics:
//     psraw:
//     switch:
// The following arguments can be used to trigger demo modes:
//     climbdemo:
//     stfdemo:
// Other arguments control vario setup:
//     save: saves the vario configuration to nonvolatile memory on the instrument
//     zero: Zero's the airspeed indicator's offset
//
void
InputEvents::eventAdjustVarioFilter(const TCHAR *misc)
{
  static int naccel = 0;
  if (_tcscmp(misc, _T("slow")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,VarioTimeConstant,3"));
  else if (_tcscmp(misc, _T("medium")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,VarioTimeConstant,2"));
  else if (_tcscmp(misc, _T("fast")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,VarioTimeConstant,1"));
  else if (_tcscmp(misc, _T("statistics")) == 0) {
    VarioWriteNMEA(_T("PDVSC,S,Diagnostics,1"));
  } else if (_tcscmp(misc, _T("diagnostics")) == 0) {
    VarioWriteNMEA(_T("PDVSC,S,Diagnostics,2"));
  } else if (_tcscmp(misc, _T("psraw")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,Diagnostics,3"));
  else if (_tcscmp(misc, _T("switch")) == 0)
    VarioWriteNMEA(_T("PDVSC,S,Diagnostics,4"));
  else if (_tcscmp(misc, _T("democlimb")) == 0) {
    VarioWriteNMEA(_T("PDVSC,S,DemoMode,0"));
    VarioWriteNMEA(_T("PDVSC,S,DemoMode,2"));
  } else if (_tcscmp(misc, _T("demostf"))==0) {
    VarioWriteNMEA(_T("PDVSC,S,DemoMode,0"));
    VarioWriteNMEA(_T("PDVSC,S,DemoMode,1"));
  } else if (_tcscmp(misc, _T("accel")) == 0) {
    switch (naccel) {
    case 0:
      VarioWriteNMEA(_T("PDVSC,R,AccelerometerSlopeX"));
      break;
    case 1:
      VarioWriteNMEA(_T("PDVSC,R,AccelerometerSlopeY"));
      break;
    case 2:
      VarioWriteNMEA(_T("PDVSC,R,AccelerometerOffsetX"));
      break;
    case 3:
      VarioWriteNMEA(_T("PDVSC,R,AccelerometerOffsetY"));
      break;
    default:
      naccel = 0;
      break;
    }
    naccel++;
    if (naccel > 3)
      naccel = 0;

  } else if (_tcscmp(misc, _T("xdemo")) == 0) {
    dlgVegaDemoShowModal();
  } else if (_tcscmp(misc, _T("zero"))==0) {
    // zero, no mixing
    if (!CommonInterface::Calculated().flight.Flying) {
      VarioWriteNMEA(_T("PDVSC,S,ZeroASI,1"));
    }
  } else if (_tcscmp(misc, _T("save")) == 0) {
    VarioWriteNMEA(_T("PDVSC,S,StoreToEeprom,2"));

  // accel calibration
  } else if (!CommonInterface::Calculated().flight.Flying) {
    if (_tcscmp(misc, _T("X1"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,1"));
    else if (_tcscmp(misc, _T("X2"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,2"));
    else if (_tcscmp(misc, _T("X3"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,3"));
    else if (_tcscmp(misc, _T("X4"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,4"));
    else if (_tcscmp(misc, _T("X5"))==0)
      VarioWriteNMEA(_T("PDVSC,S,CalibrateAccel,5"));
  }
}

// Adjust audio deadband of internal vario sounds
// +: increases deadband
// -: decreases deadband
void
InputEvents::eventAudioDeadband(const TCHAR *misc)
{
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();

  if (_tcscmp(misc, _T("+"))) {
    if (settings_computer.SoundDeadband >= 40)
      return;

    settings_computer.SoundDeadband++;
  }
  if (_tcscmp(misc, _T("-"))) {
    if (settings_computer.SoundDeadband <= 0)
      return;

    settings_computer.SoundDeadband--;
  }

  /*
  VarioSound_SetVdead(SoundDeadband);
  */

  Profile::SetSoundSettings(); // save to registry

  // TODO feature: send to vario if available
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
    protected_task_manager->incrementActiveTaskPoint(1); // next
  else if (_tcscmp(misc, _T("nextwrap")) == 0)
    protected_task_manager->incrementActiveTaskPoint(1); // next - with wrap
  else if (_tcscmp(misc, _T("previous")) == 0)
    protected_task_manager->incrementActiveTaskPoint(-1); // previous
  else if (_tcscmp(misc, _T("previouswrap")) == 0)
    protected_task_manager->incrementActiveTaskPoint(-1); // previous with wrap
  else if (_tcscmp(misc, _T("nextarm")) == 0)
    protected_task_manager->incrementActiveTaskPointArm(1); // arm sensitive next
  else if (_tcscmp(misc, _T("previousarm")) == 0)
    protected_task_manager->incrementActiveTaskPointArm(-1); // arm sensitive previous

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
    task_manager->abort();
  else if (_tcscmp(misc, _T("resume")) == 0)
    task_manager->resume();
  else if (_tcscmp(misc, _T("show")) == 0) {
    switch (task_manager->get_mode()) {
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
    switch (task_manager->get_mode()) {
    case TaskManager::MODE_NULL:
    case TaskManager::MODE_ORDERED:
      task_manager->abort();
      break;
    case TaskManager::MODE_GOTO:
      if (task_manager->check_ordered_task()) {
        task_manager->resume();
      } else {
        task_manager->abort();
      }
      break;
    case TaskManager::MODE_ABORT:
      task_manager->resume();
      break;
    default:
      break;
    }
  }

  trigger_redraw();
}

// Bugs
// Adjusts the degradation of glider performance due to bugs
// up: increases the performance by 10%
// down: decreases the performance by 10%
// max: cleans the aircraft of bugs
// min: selects the worst performance (50%)
// show: shows the current bug degradation
void
InputEvents::eventBugs(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  GlidePolar &polar = CommonInterface::SetSettingsComputer().glide_polar_task;
  fixed BUGS = polar.GetBugs();
  fixed oldBugs = BUGS;

  if (_tcscmp(misc, _T("up")) == 0) {
    BUGS += fixed_one / 10;
    if (BUGS > fixed_one)
      BUGS = fixed_one;
  } else if (_tcscmp(misc, _T("down")) == 0) {
    BUGS -= fixed_one / 10;
    if (BUGS < fixed_half)
      BUGS = fixed_half;
  } else if (_tcscmp(misc, _T("max")) == 0)
    BUGS = fixed_one;
  else if (_tcscmp(misc, _T("min")) == 0)
    BUGS = fixed_half;
  else if (_tcscmp(misc, _T("show")) == 0) {
    TCHAR Temp[100];
    _stprintf(Temp, _T("%d"), (int)(BUGS * 100));
    Message::AddMessage(_("Bugs performance"), Temp);
  }

  if (BUGS != oldBugs) {
    polar.SetBugs(fixed(BUGS));
    protected_task_manager->set_glide_polar(polar);
  }
}

// Ballast
// Adjusts the ballast setting of the glider
// up: increases ballast by 10%
// down: decreases ballast by 10%
// max: selects 100% ballast
// min: selects 0% ballast
// show: displays a status message indicating the ballast percentage
void
InputEvents::eventBallast(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  GlidePolar &polar = CommonInterface::SetSettingsComputer().glide_polar_task;
  fixed BALLAST = polar.GetBallast();
  fixed oldBallast = BALLAST;

  if (_tcscmp(misc, _T("up")) == 0) {
    BALLAST += fixed_one / 10;
    if (BALLAST >= fixed_one)
      BALLAST = fixed_one;
  } else if (_tcscmp(misc, _T("down")) == 0) {
    BALLAST -= fixed_one / 10;
    if (BALLAST < fixed_zero)
      BALLAST = fixed_zero;
  } else if (_tcscmp(misc, _T("max")) == 0)
    BALLAST = fixed_one;
  else if (_tcscmp(misc, _T("min")) == 0)
    BALLAST = fixed_zero;
  else if (_tcscmp(misc, _T("show")) == 0) {
    TCHAR Temp[100];
    _stprintf(Temp, _T("%d"), (int)(BALLAST * 100));
    Message::AddMessage(_("Ballast %"), Temp);
  }

  if (BALLAST != oldBallast) {
    polar.SetBallast(fixed(BALLAST));
    protected_task_manager->set_glide_polar(polar);
  }
}

void
InputEvents::eventAutoLogger(const TCHAR *misc)
{
  if (!XCSoarInterface::SettingsComputer().DisableAutoLogger)
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

  const NMEA_INFO &basic = CommonInterface::Basic();
  const SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SettingsComputer();

  if (_tcscmp(misc, _T("start ask")) == 0)
    logger.guiStartLogger(basic, settings_computer,
                          *protected_task_manager);
  else if (_tcscmp(misc, _T("start")) == 0)
    logger.guiStartLogger(basic, settings_computer,
                          *protected_task_manager, true);
  else if (_tcscmp(misc, _T("stop ask")) == 0)
    logger.guiStopLogger(basic);
  else if (_tcscmp(misc, _T("stop")) == 0)
    logger.guiStopLogger(basic, true);
  else if (_tcscmp(misc, _T("toggle ask")) == 0)
    logger.guiToggleLogger(basic, settings_computer,
                           *protected_task_manager);
  else if (_tcscmp(misc, _T("toggle")) == 0)
    logger.guiToggleLogger(basic, settings_computer,
                           *protected_task_manager, true);
  else if (_tcscmp(misc, _T("nmea")) == 0) {
    EnableLogNMEA = !EnableLogNMEA;
    if (EnableLogNMEA) {
      Message::AddMessage(_("NMEA log on"));
    } else {
      Message::AddMessage(_("NMEA log off"));
    }
  } else if (_tcscmp(misc, _T("download")) == 0) {
    ExternalLogger::DownloadFlight();
  } else if (_tcscmp(misc, _T("show")) == 0)
    if (logger.isLoggerActive()) {
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

// NearestAirspaceDetails
// Displays details of the nearest airspace to the aircraft in a
// status message.  This does nothing if there is no airspace within
// 100km of the aircraft.
// If the aircraft is within airspace, this displays the distance and bearing
// to the nearest exit to the airspace.

void 
InputEvents::eventNearestAirspaceDetails(gcc_unused const TCHAR *misc)
{
  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SettingsComputer();

  if (!airspace_warnings->warning_empty()) {
    // Prevent the dialog from closing itself without active warning
    // This is relevant if there are only acknowledged airspaces in the list
    // AutoClose will be reset when the dialog is closed again by hand
    dlgAirspaceWarningsShowModal(XCSoarInterface::main_window);
    return;
  }

  const AIRCRAFT_STATE aircraft_state =
    ToAircraftState(basic, calculated);
  AirspaceVisible visible(settings_computer.airspace,
                          CommonInterface::SettingsMap().airspace,
                          aircraft_state);
  GlidePolar polar = settings_computer.glide_polar_task;
  polar.SetMC(max(polar.GetMC(),fixed_one));
  AirspaceAircraftPerformanceGlide perf(polar);
  AirspaceSoonestSort ans(aircraft_state, perf, fixed(1800), visible);

  const AbstractAirspace* as = ans.find_nearest(airspace_database);
  if (!as) {
    return;
  } 

  dlgAirspaceDetails(*as);

  // clear previous warning if any
  XCSoarInterface::main_window.popup.Acknowledge(PopupMessage::MSG_AIRSPACE);

  // TODO code: No control via status data (ala DoStatusMEssage)
  // - can we change this?
//  Message::AddMessage(5000, Message::MSG_AIRSPACE, text);
}

// NearestWaypointDetails
// Displays the waypoint details dialog
//  aircraft: the waypoint nearest the aircraft
//  pan: the waypoint nearest to the pan cursor
void
InputEvents::eventNearestWaypointDetails(const TCHAR *misc)
{
  const NMEA_INFO &basic = CommonInterface::Basic();
  const GlueMapWindow *map_window = CommonInterface::main_window.map;
  if (map_window == NULL)
    return;

  if (_tcscmp(misc, _T("aircraft")) == 0)
    // big range..
    PopupNearestWaypointDetails(way_points, basic.Location,
                                1.0e5);
  else if (_tcscmp(misc, _T("pan")) == 0)
    // big range..
    PopupNearestWaypointDetails(way_points,
                                map_window->VisibleProjection().GetGeoLocation(),
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
    protected_task_manager->task_load(buffer, &way_points);
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
    protected_task_manager->task_save(buffer);
  }
}

// ProfileLoad
// Loads the profile of the specified filename
void
InputEvents::eventProfileLoad(const TCHAR *misc)
{
  if (!string_is_empty(misc)) {
    Profile::LoadFile(misc);

    WaypointFileChanged = true;
    TerrainFileChanged = true;
    TopographyFileChanged = true;
    AirspaceFileChanged = true;
    AirfieldFileChanged = true;
    PolarFileChanged = true;

    // assuming all is ok, we can...
    Profile::Use();
  }
}

// ProfileSave
// Saves the profile to the specified filename
void
InputEvents::eventProfileSave(const TCHAR *misc)
{
  if (!string_is_empty(misc))
    Profile::SaveFile(misc);
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

// AdjustForecastTemperature
// Adjusts the maximum ground temperature used by the convection forecast
// +: increases temperature by one degree celsius
// -: decreases temperature by one degree celsius
// show: Shows a status message with the current forecast temperature
void
InputEvents::eventAdjustForecastTemperature(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("+")) == 0)
    CuSonde::adjustForecastTemperature(1.0);
  else if (_tcscmp(misc, _T("-")) == 0)
    CuSonde::adjustForecastTemperature(-1.0);
  else if (_tcscmp(misc, _T("show")) == 0) {
    TCHAR Temp[100];
    _stprintf(Temp, _T("%f"), CuSonde::maxGroundTemperature);
    Message::AddMessage(_("Forecast temperature"), Temp);
  }
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
InputEvents::eventDeclutterLabels(const TCHAR *misc)
{
  static const TCHAR *const msg[] = {N_("All"),
                                     N_("Task & Landables"),
                                     N_("Task"),
                                     N_("None")};
  static const unsigned int n=sizeof(msg)/sizeof(msg[0]);
  static const TCHAR *const actions[n] = {_T("all"),
                                          _T("task+landables"),
                                          _T("task"),
                                          _T("none")};

  WaypointLabelSelection_t &wls =
    XCSoarInterface::SetSettingsMap().waypoint.label_selection;
  if (_tcscmp(misc, _T("toggle")) == 0)
    wls = (WaypointLabelSelection_t) ((wls + 1) %  n);
  else if (_tcscmp(misc, _T("show")) == 0 && (unsigned int) wls < n) {
    TCHAR tbuf[64];
    _stprintf(tbuf, _T("%s: %s"), _("Waypoint labels"), gettext(msg[wls]));
    Message::AddMessage(tbuf);
  }
  else {
    for (unsigned int i=0; i<n; i++)
      if (_tcscmp(misc, actions[i]) == 0)
        wls = (WaypointLabelSelection_t) i;
  }

  ActionInterface::SendSettingsMap(true);
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
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (_tcscmp(misc, _T("unforce")) == 0)
    settings_map.UserForceDisplayMode = DM_NONE;
  else if (_tcscmp(misc, _T("forceclimb")) == 0)
    settings_map.UserForceDisplayMode = DM_CIRCLING;
  else if (_tcscmp(misc, _T("forcecruise")) == 0)
    settings_map.UserForceDisplayMode = DM_CRUISE;
  else if (_tcscmp(misc, _T("forcefinal")) == 0)
    settings_map.UserForceDisplayMode = DM_FINAL_GLIDE;
  else if (_tcscmp(misc, _T("show")) == 0)
    Message::AddMessage(_("Map labels on"));

  ActionInterface::SendSettingsMap(true);
}

void
InputEvents::eventAirspaceDisplayMode(const TCHAR *misc)
{
  AirspaceRendererSettings &settings =
    CommonInterface::SetSettingsMap().airspace;

  if (_tcscmp(misc, _T("all")) == 0)
    settings.altitude_mode = ALLON;
  else if (_tcscmp(misc, _T("clip")) == 0)
    settings.altitude_mode = CLIP;
  else if (_tcscmp(misc, _T("auto")) == 0)
    settings.altitude_mode = AUTO;
  else if (_tcscmp(misc, _T("below")) == 0)
    settings.altitude_mode = ALLBELOW;
  else if (_tcscmp(misc, _T("off")) == 0)
    settings.altitude_mode = ALLOFF;

  trigger_redraw();
}

void
InputEvents::eventAddWaypoint(const TCHAR *misc)
{
  const NMEA_INFO &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (_tcscmp(misc, _T("takeoff")) == 0) {
    if (basic.LocationAvailable && calculated.TerrainValid) {
      ScopeSuspendAllThreads suspend;
      way_points.add_takeoff_point(basic.Location, calculated.TerrainAlt);
    }
  } else {
    Waypoint edit_waypoint = way_points.create(basic.Location);
    edit_waypoint.Altitude = calculated.TerrainAlt;
    if (!dlgWaypointEditShowModal(edit_waypoint) || edit_waypoint.Name.empty()) {
      trigger_redraw();
      return;
    }
    {
      ScopeSuspendAllThreads suspend;
      way_points.append(edit_waypoint);
    }
  }

  trigger_redraw();
}

void
InputEvents::eventOrientation(const TCHAR *misc)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (_tcscmp(misc, _T("northup")) == 0) {
    settings_map.OrientationCruise = NORTHUP;
    settings_map.OrientationCircling = NORTHUP;
  } else if (_tcscmp(misc, _T("northcircle")) == 0) {
    settings_map.OrientationCruise = TRACKUP;
    settings_map.OrientationCircling = NORTHUP;
  } else if (_tcscmp(misc, _T("trackcircle")) == 0) {
    settings_map.OrientationCruise = NORTHUP;
    settings_map.OrientationCircling = TRACKUP;
  } else if (_tcscmp(misc, _T("trackup")) == 0) {
    settings_map.OrientationCruise = TRACKUP;
    settings_map.OrientationCircling = TRACKUP;
  } else if (_tcscmp(misc, _T("northtrack")) == 0) {
    settings_map.OrientationCruise = TRACKUP;
    settings_map.OrientationCircling = TARGETUP;
  }

  ActionInterface::SendSettingsMap(true);
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

/* Event_TerrainToplogy Changes
   0       Show
   1       Topography = ON
   2       Topography = OFF
   3       Terrain = ON
   4       Terrain = OFF
   -1      Toggle through 4 stages (off/off, off/on, on/off, on/on)
   -2      Toggle terrain
   -3      Toggle topography
 */

void
InputEvents::sub_TerrainTopography(int vswitch)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (vswitch == -1) {
    // toggle through 4 possible options
    char val = 0;

    if (settings_map.EnableTopography)
      val++;
    if (settings_map.terrain.enable)
      val += (char)2;

    val++;
    if (val > 3)
      val = 0;

    settings_map.EnableTopography = ((val & 0x01) == 0x01);
    settings_map.terrain.enable = ((val & 0x02) == 0x02);
  } else if (vswitch == -2)
    // toggle terrain
    settings_map.terrain.enable = !settings_map.terrain.enable;
  else if (vswitch == -3)
    // toggle topography
    settings_map.EnableTopography = !settings_map.EnableTopography;
  else if (vswitch == 1)
    // Turn on topography
    settings_map.EnableTopography = true;
  else if (vswitch == 2)
    // Turn off topography
    settings_map.EnableTopography = false;
  else if (vswitch == 3)
    // Turn on terrain
    settings_map.terrain.enable = true;
  else if (vswitch == 4)
    // Turn off terrain
    settings_map.terrain.enable = false;
  else if (vswitch == 0) {
    // Show terrain/topography
    // ARH Let user know what's happening
    TCHAR buf[128];

    if (settings_map.EnableTopography)
      _stprintf(buf, _T("\r\n%s / "), _("On"));
    else
      _stprintf(buf, _T("\r\n%s / "), _("Off"));

    _tcscat(buf, settings_map.terrain.enable
            ? _("On") : _("Off"));

    Message::AddMessage(_("Topography/Terrain"), buf);
    return;
  }

  /* save new values to profile */
  Profile::Set(szProfileDrawTopography,
               settings_map.EnableTopography);
  Profile::Set(szProfileDrawTerrain,
               settings_map.terrain.enable);

  XCSoarInterface::SendSettingsMap(true);
}

/**
 * This function switches the pan mode on and off
 * @param vswitch This parameter determines what to do:
 * -2 supertoogle
 * -1 toogle
 * 1  on
 * 0  off
 * @see InputEvents::eventPan()
 */
void
InputEvents::sub_Pan(int vswitch)
{
  GlueMapWindow *map_window = CommonInterface::main_window.map;
  if (map_window == NULL)
    return;

  bool oldPan = map_window->IsPanning();

  if (vswitch == -2) {
    // supertoogle, toogle pan mode and fullscreen
    map_window->TogglePan();
    XCSoarInterface::main_window.SetFullScreen(true);
  } else if (vswitch == -1)
    // toogle, toogle pan mode only
    map_window->TogglePan();
  else
    // 1 = enable pan mode
    // 0 = disable pan mode
    map_window->SetPan(vswitch != 0);

  if (map_window->IsPanning() != oldPan) {
    if (map_window->IsPanning()) {
      setMode(MODE_PAN);
    } else {
      setMode(MODE_DEFAULT);
      Pages::Update();
    }
  }
}

void
InputEvents::sub_PanCursor(int dx, int dy)
{
  GlueMapWindow *map_window = CommonInterface::main_window.map;
  if (map_window == NULL || !map_window->IsPanning())
    return;

  const WindowProjection &projection = map_window->VisibleProjection();

  RasterPoint pt = projection.GetScreenOrigin();
  pt.x -= dx * projection.GetScreenWidth() / 4;
  pt.y -= dy * projection.GetScreenHeight() / 4;
  map_window->SetLocation(projection.ScreenToGeo(pt));

  map_window->QuickRedraw();
}

// called from UI or input event handler (same thread)
void
InputEvents::sub_AutoZoom(int vswitch)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  if (vswitch == -1)
    settings_map.AutoZoom = !settings_map.AutoZoom;
  else
    settings_map.AutoZoom = (vswitch != 0); // 0 off, 1 on

  Profile::Set(szProfileAutoZoom, settings_map.AutoZoom);

  if (settings_map.AutoZoom &&
      CommonInterface::main_window.map != NULL)
    CommonInterface::main_window.map->SetPan(false);

  ActionInterface::SendSettingsMap(true);
}

void
InputEvents::sub_SetZoom(fixed value)
{
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();
  GlueMapWindow *map_window = CommonInterface::main_window.map;
  if (map_window == NULL)
    return;

  DisplayMode displayMode = XCSoarInterface::main_window.GetDisplayMode();
  if (settings_map.AutoZoom &&
      !(displayMode == DM_CIRCLING && settings_map.CircleZoom) &&
      !CommonInterface::IsPanning()) {
    settings_map.AutoZoom = false;  // disable autozoom if user manually changes zoom
    Profile::Set(szProfileAutoZoom, false);
    Message::AddMessage(_("Auto. zoom off"));
  }

  fixed vmin = CommonInterface::SettingsComputer().glide_polar_task.GetVMin();
  fixed scale_2min_distance = vmin * fixed_int_constant(12);
  const fixed scale_500m = fixed_int_constant(50);
  const fixed scale_1600km = fixed_int_constant(1600*100);
  fixed minreasonable = (displayMode == DM_CIRCLING) ?
                        scale_500m : max(scale_500m, scale_2min_distance);

  value = max(minreasonable, min(scale_1600km, value));
  map_window->SetMapScale(value);
  map_window->QuickRedraw();
}

void
InputEvents::sub_ScaleZoom(int vswitch)
{
  const GlueMapWindow *map_window = CommonInterface::main_window.map;
  if (map_window == NULL)
    return;

  const MapWindowProjection &projection =
      map_window->VisibleProjection();

  fixed value = projection.GetMapScale();

  if (projection.HaveScaleList()) {
    value = projection.StepMapScale(value, -vswitch);
  } else {
    if (vswitch == 1)
      // zoom in a little
      value /= fixed_sqrt_two;
    else if (vswitch == -1)
      // zoom out a little
      value *= fixed_sqrt_two;
    else if (vswitch == 2)
      // zoom in a lot
      value /= 2;
    else if (vswitch == -2)
      // zoom out a lot
      value *= 2;
  }

  sub_SetZoom(value);
}

void
InputEvents::eventTaskTransition(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return;

  if (_tcscmp(misc, _T("start")) == 0) {
    AIRCRAFT_STATE start_state = protected_task_manager->get_start_state();

    TCHAR TempTime[40];
    TCHAR TempAlt[40];
    TCHAR TempSpeed[40];
    
    Units::TimeToTextHHMMSigned(TempTime, (int)TimeLocal((int)start_state.Time));
    Units::FormatUserAltitude(start_state.NavAltitude,
                              TempAlt, sizeof(TempAlt)/sizeof(TCHAR), true);
    Units::FormatUserSpeed(start_state.Speed,
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

void
InputEvents::eventWeather(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("list")) == 0)
    dlgNOAAListShowModal(XCSoarInterface::main_window);
}
