// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "MapSettings.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Engine/Airspace/AirspaceAircraftPerformance.hpp"
#include "Engine/Airspace/SoonestAirspace.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
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
    CommonInterface::SetMapSettings().airspace;

  if (StringIsEqual(misc, _T("toggle")))
    settings.enable = !settings.enable;
  else if (StringIsEqual(misc, _T("off")))
    settings.enable = false;
  else if (StringIsEqual(misc, _T("on")))
    settings.enable = true;
  else if (StringIsEqual(misc, _T("show"))) {
    if (!settings.enable)
      Message::AddMessage(_("Show airspace off"));
    if (settings.enable)
      Message::AddMessage(_("Show airspace on"));
    return;
  } else if (StringIsEqual(misc, _T("list"))) {
    ShowAirspaceListDialog(*data_components->airspaces,
                           backend_components->GetAirspaceWarnings());
    return;
  }

  ActionInterface::SendMapSettings(true);
}

// ClearAirspaceWarnings
// Clears airspace warnings for the selected airspace
void
InputEvents::eventClearAirspaceWarnings([[maybe_unused]] const TCHAR *misc)
{
  if (auto *airspace_warnings = backend_components->GetAirspaceWarnings())
    airspace_warnings->AcknowledgeAll();
}

// NearestAirspaceDetails
// Displays details of the nearest airspace to the aircraft in a
// status message.  This does nothing if there is no airspace within
// 100km of the aircraft.
// If the aircraft is within airspace, this displays the distance and bearing
// to the nearest exit to the airspace.

void 
InputEvents::eventNearestAirspaceDetails([[maybe_unused]] const TCHAR *misc)
{
  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  auto *airspace_warnings = backend_components->GetAirspaceWarnings();
  if (airspace_warnings != nullptr && !airspace_warnings->IsEmpty()) {
    // Prevent the dialog from closing itself without active warning
    // This is relevant if there are only acknowledged airspaces in the list
    // AutoClose will be reset when the dialog is closed again by hand
    dlgAirspaceWarningsShowModal(*airspace_warnings);
    return;
  }

  const AircraftState aircraft_state =
    ToAircraftState(basic, calculated);
  auto visible = AirspaceVisiblePredicate(settings_computer.airspace,
                                          CommonInterface::GetMapSettings().airspace,
                                          aircraft_state);
  GlidePolar polar = settings_computer.polar.glide_polar_task;
  polar.SetMC(std::max(polar.GetMC(), 1.));
  const AirspaceAircraftPerformance perf(polar);

  const auto as = FindSoonestAirspace(*data_components->airspaces, aircraft_state, perf,
                                      std::move(visible),
                                      std::chrono::minutes{30});
  if (!as) {
    return;
  } 

  dlgAirspaceDetails(std::move(as), airspace_warnings);
}
