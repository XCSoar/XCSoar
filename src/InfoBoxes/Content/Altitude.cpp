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

#include "InfoBoxes/Content/Altitude.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Units/Units.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#include "DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Simulator.hpp"
#include "Protection.hpp"
#include "MainWindow.hpp"

#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Screen/Layout.hpp"
#include "DataField/Float.hpp"
#include "DataField/Base.hpp"
#include "Util/Macros.hpp"

#include <tchar.h>
#include <stdio.h>

static gcc_constexpr_data
CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(InfoBoxContentAltitude::PnlSimulatorOnPlusBig),
  DeclareCallBackEntry(InfoBoxContentAltitude::PnlSimulatorOnPlusSmall),
  DeclareCallBackEntry(InfoBoxContentAltitude::PnlSimulatorOnMinusSmall),
  DeclareCallBackEntry(InfoBoxContentAltitude::PnlSimulatorOnMinusBig),
  DeclareCallBackEntry(InfoBoxContentAltitude::PnlSetupOnQNH),
  DeclareCallBackEntry(InfoBoxContentAltitude::PnlSetupOnSetup),

  DeclareCallBackEntry(NULL)
};

/*
 * InfoBoxContentAltitude
 *
 * Subpart Panel Info
 */

static int InfoBoxID;

Window*
InfoBoxContentAltitude::PnlInfoLoad(SingleWindow &parent, TabBarControl* wTabBar,
                                    WndForm* wf, const int id)
{
  assert(wTabBar);
  assert(wf);
//  wf = _wf;

  InfoBoxID = id;

  Window *wInfoBoxAccessInfo =
      LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_INFOBOXALTITUDEINFO"));
  assert(wInfoBoxAccessInfo);

  wf->SetTimerNotify(OnTimerNotify);

  return wInfoBoxAccessInfo;
}

void
InfoBoxContentAltitude::OnTimerNotify(gcc_unused WndForm &Sender)
{
  PnlInfoUpdate();
}

bool
InfoBoxContentAltitude::PnlInfoOnTabPreShow(TabBarControl::EventType EventType)
{
  return PnlInfoUpdate();
}

bool
InfoBoxContentAltitude::PnlInfoUpdate()
{
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const NMEAInfo &basic = CommonInterface::Basic();
  TCHAR sTmp[32];

  if (!calculated.altitude_agl_valid) {
    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltAGL")))->SetText(_("N/A"));
  } else {
    // Set Value
    _stprintf(sTmp, _T("%.0f %s"), (double)Units::ToUserAltitude(calculated.altitude_agl),
                                   Units::GetAltitudeName());

    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltAGL")))->SetText(sTmp);
  }

  if (!basic.baro_altitude_available) {
    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltBaro")))->SetText(_("N/A"));
  } else {
    // Set Value
    _stprintf(sTmp, _T("%.0f %s"), (double)Units::ToUserAltitude(basic.baro_altitude),
                                       Units::GetAltitudeName());

    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltBaro")))->SetText(sTmp);
  }

  if (!basic.gps_altitude_available) {
    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltGPS")))->SetText(_("N/A"));
  } else {
    // Set Value
     _stprintf(sTmp, _T("%.0f %s"), (double)Units::ToUserAltitude(basic.gps_altitude),
                                         Units::GetAltitudeName());

      ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltGPS")))->SetText(sTmp);
  }

  if (!calculated.terrain_valid){
    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpTerrain")))->SetText(_("N/A"));
  } else {
    // Set Value
     _stprintf(sTmp, _T("%.0f %s"), (double)Units::ToUserAltitude(calculated.terrain_altitude),
                                         Units::GetAltitudeName());

      ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpTerrain")))->SetText(sTmp);
  }

  return true;
}

/*
 * Subpart Panel Simulator
 */

Window*
InfoBoxContentAltitude::PnlSimulatorLoad(SingleWindow &parent,
                                         TabBarControl* wTabBar,
                                         WndForm* wf, const int id)
{
  assert(wTabBar);
  assert(wf);

  if (!is_simulator())
    return NULL;

  InfoBoxID = id;

  Window *wInfoBoxAccessSimulator =
      LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_INFOBOXALTITUDESIMULATOR"));
  assert(wInfoBoxAccessSimulator);

  return wInfoBoxAccessSimulator;
}

void
InfoBoxContentAltitude::PnlSimulatorOnPlusBig(gcc_unused WndButton &Sender)
{
  if (!is_simulator())
    return;

  ChangeAltitude(fixed(+100));
}

void
InfoBoxContentAltitude::PnlSimulatorOnPlusSmall(gcc_unused WndButton &Sender)
{
  if (!is_simulator())
    return;

  ChangeAltitude(fixed(+10));
}

void
InfoBoxContentAltitude::PnlSimulatorOnMinusSmall(gcc_unused WndButton &Sender)
{
  if (!is_simulator())
    return;

  ChangeAltitude(fixed(-10));
}

void
InfoBoxContentAltitude::PnlSimulatorOnMinusBig(gcc_unused WndButton &Sender)
{
  if (!is_simulator())
    return;

  ChangeAltitude(fixed(-100));
}

void
InfoBoxContentAltitude::ChangeAltitude(const fixed step)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  device_blackboard->SetAltitude(basic.gps_altitude +
                                 (fixed)Units::ToSysAltitude(step));
}

/*
 * Subpart Panel Setup
 */

Window*
InfoBoxContentAltitude::PnlSetupLoad(SingleWindow &parent, TabBarControl* wTabBar,
                                     WndForm* wf, const int id)
{
  assert(wTabBar);
  assert(wf);
//  wf = _wf;

  InfoBoxID = id;

  Window *wInfoBoxAccessSetup =
      LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_INFOBOXALTITUDESETUP"));
  assert(wInfoBoxAccessSetup);

  LoadFormProperty(*wf, _T("prpQNH"),
                   CommonInterface::SettingsComputer().pressure.GetHectoPascal());

  return wInfoBoxAccessSetup;
}

void
InfoBoxContentAltitude::PnlSetupOnQNH(DataField *_Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat *Sender = (DataFieldFloat *)_Sender;
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();

  switch (Mode) {
  case DataField::daChange:
    settings_computer.pressure = AtmosphericPressure::HectoPascal(Sender->GetAsFixed());
    settings_computer.pressure_available.Update(CommonInterface::Basic().clock);
    device_blackboard->SetQNH(Sender->GetAsFixed());
    break;

  case DataField::daSpecial:
    return;
  }
}

void
InfoBoxContentAltitude::PnlSetupOnSetup(gcc_unused WndButton &Sender)
{
  InfoBoxManager::SetupFocused(InfoBoxID);
  dlgInfoBoxAccess::OnClose();
}

/*
 * Subpart callback function pointers
 */

static gcc_constexpr_data
InfoBoxContentAltitude::PanelContent Panels[] = {
  InfoBoxContentAltitude::PanelContent (
    N_("Simulator"),
    (*InfoBoxContentAltitude::PnlSimulatorLoad)),

  InfoBoxContentAltitude::PanelContent (
    N_("Info"),
    (*InfoBoxContentAltitude::PnlInfoLoad),
    NULL,
    (*InfoBoxContentAltitude::PnlInfoOnTabPreShow)),

  InfoBoxContentAltitude::PanelContent (
    N_("Setup"),
    (*InfoBoxContentAltitude::PnlSetupLoad))
};

static gcc_constexpr_data
InfoBoxContentAltitude::DialogContent dlgContent = {
  ARRAY_SIZE(Panels), &Panels[0],
  &CallBackTable[0],
};

const InfoBoxContentAltitude::DialogContent *
InfoBoxContentAltitude::GetDialogContent() {
  return &dlgContent;
}

void
InfoBoxContentAltitudeGPS::Update(InfoBoxWindow &infobox)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  TCHAR sTmp[32];

  if (!basic.gps_altitude_available) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Units::FormatUserAltitude(basic.gps_altitude, sTmp,
                            ARRAY_SIZE(sTmp), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(basic.gps_altitude, sTmp,
                                     ARRAY_SIZE(sTmp));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::current.altitude_unit);
}

bool
InfoBoxContentAltitudeGPS::HandleKey(const InfoBoxKeyCodes keycode)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (!is_simulator())
    return false;
  if (!basic.gps.simulator)
    return false;

  const Angle a5 = Angle::Degrees(fixed(5));

  switch (keycode) {
  case ibkUp:
    ChangeAltitude(fixed(+100));
    return true;

  case ibkDown:
    ChangeAltitude(fixed(-100));
    return true;

  case ibkLeft:
    device_blackboard->SetTrack(
        basic.track - a5);
    return true;

  case ibkRight:
    device_blackboard->SetTrack(
        basic.track + a5);
    return true;

  case ibkEnter:
    break;
  }

  return false;
}

void
InfoBoxContentAltitudeAGL::Update(InfoBoxWindow &infobox)
{
  const DerivedInfo &calculated = CommonInterface::Calculated();
  TCHAR sTmp[32];

  if (!calculated.altitude_agl_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Units::FormatUserAltitude(calculated.altitude_agl, sTmp,
                            ARRAY_SIZE(sTmp), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(calculated.altitude_agl, sTmp,
                                     ARRAY_SIZE(sTmp));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::current.altitude_unit);

  // Set Color (red/black)
  infobox.SetColor(calculated.altitude_agl <
      XCSoarInterface::SettingsComputer().task.route_planner.safety_height_terrain ? 1 : 0);
}

void
InfoBoxContentAltitudeBaro::Update(InfoBoxWindow &infobox)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  TCHAR sTmp[32];

  if (!basic.baro_altitude_available) {
    infobox.SetInvalid();

    if (basic.pressure_altitude_available)
      infobox.SetComment(_("no QNH"));

    return;
  }

  // Set Value
  Units::FormatUserAltitude(basic.baro_altitude, sTmp,
                            ARRAY_SIZE(sTmp), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(basic.baro_altitude, sTmp,
                                     ARRAY_SIZE(sTmp));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxContentAltitudeQFE::Update(InfoBoxWindow &infobox)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  TCHAR sTmp[32];

  if (!basic.gps_altitude_available) {
    infobox.SetInvalid();
    return;
  }

  fixed Value = basic.gps_altitude;

  const Waypoint *home_waypoint = way_points.GetHome();
  if (home_waypoint)
    Value -= home_waypoint->altitude;

  // Set Value
  Units::FormatUserAltitude(Value, sTmp,
                            ARRAY_SIZE(sTmp), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(Value, sTmp,
                                     ARRAY_SIZE(sTmp));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::current.altitude_unit);
}

void
InfoBoxContentFlightLevel::Update(InfoBoxWindow &infobox)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SettingsComputer();
  TCHAR sTmp[32];

  if (basic.pressure_altitude_available) {
    fixed Altitude = Units::ToUserUnit(basic.pressure_altitude, unFeet);

    // Title color black
    infobox.SetColorTop(0);

    // Set Value
    _stprintf(sTmp, _T("%03d"), iround(Altitude/100));
    infobox.SetValue(sTmp);

    // Set Comment
    _stprintf(sTmp, _T("%dft"), iround(Altitude));
    infobox.SetComment(sTmp);

  } else if (basic.gps_altitude_available &&
             settings_computer.pressure_available) {
    // Take gps altitude as baro altitude. This is inaccurate but still fits our needs.
    const AtmosphericPressure &qnh = settings_computer.pressure;
    fixed Altitude = Units::ToUserUnit(qnh.QNHAltitudeToPressureAltitude(basic.gps_altitude), unFeet);

    // Title color red
    infobox.SetColorTop(1);

    // Set Value
    _stprintf(sTmp, _T("%03d"), iround(Altitude/100));
    infobox.SetValue(sTmp);

    // Set Comment
    _stprintf(sTmp, _T("%dft"), iround(Altitude));
    infobox.SetComment(sTmp);

  } else if ((basic.baro_altitude_available || basic.gps_altitude_available) &&
             !settings_computer.pressure_available) {
    infobox.SetInvalid();
    infobox.SetComment(_("no QNH"));
  } else {
    infobox.SetInvalid();
  }
}

void
InfoBoxContentTerrainHeight::Update(InfoBoxWindow &infobox)
{
  const DerivedInfo &calculated = CommonInterface::Calculated();
  TCHAR sTmp[32];

  if (!calculated.terrain_valid){
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Units::FormatUserAltitude(calculated.terrain_altitude, sTmp,
                            ARRAY_SIZE(sTmp), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(calculated.terrain_altitude, sTmp,
                                     ARRAY_SIZE(sTmp));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::current.altitude_unit);
}

