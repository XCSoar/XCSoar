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
#include "Language/Language.hpp"
#include "Message.hpp"
#include "SettingsMap.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AirspaceSoonestSort.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceAircraftPerformance.hpp"
#include "Dialogs/Airspace.hpp"
#include "Dialogs/AirspaceWarningDialog.hpp"
#include "MainWindow.hpp"
#include "NMEA/Aircraft.hpp"

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

// ClearAirspaceWarnings
// Clears airspace warnings for the selected airspace
void
InputEvents::eventClearAirspaceWarnings(gcc_unused const TCHAR *misc)
{
  if (airspace_warnings != NULL)
    airspace_warnings->clear_warnings();
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

  const AircraftState aircraft_state =
    ToAircraftState(basic, calculated);
  AirspaceVisiblePredicate visible(settings_computer.airspace,
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
