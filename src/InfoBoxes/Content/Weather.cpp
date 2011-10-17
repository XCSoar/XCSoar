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

#include "InfoBoxes/Content/Weather.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Interface.hpp"
#include "Atmosphere/CuSonde.hpp"
#include "Protection.hpp"

#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Profile/Profile.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "DataField/Boolean.hpp"
#include "Util/Macros.hpp"

#include <tchar.h>
#include <stdio.h>

static gcc_constexpr_data
CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(InfoBoxContentWind::PnlEditOnWindSpeed),
  DeclareCallBackEntry(InfoBoxContentWind::PnlEditOnWindDirection),

  DeclareCallBackEntry(InfoBoxContentWind::PnlSetupOnSetup),

  DeclareCallBackEntry(NULL)
};

void
InfoBoxContentHumidity::Update(InfoBoxWindow &infobox)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  if (!basic.humidity_available) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%d"), (int)basic.humidity);
  infobox.SetValue(tmp);
}

void
InfoBoxContentTemperature::Update(InfoBoxWindow &infobox)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  if (!basic.temperature_available) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.1f")_T(DEG),
                    Units::ToUserTemperature(basic.temperature));
}

void
InfoBoxContentTemperatureForecast::Update(InfoBoxWindow &infobox)
{
  // Set Value
  SetValueFromFixed(infobox, _T("%2.1f")_T(DEG), CuSonde::maxGroundTemperature);
}

bool
InfoBoxContentTemperatureForecast::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch(keycode) {
  case ibkUp:
    CuSonde::adjustForecastTemperature(fixed_half);
    return true;

  case ibkDown:
    CuSonde::adjustForecastTemperature(-fixed_half);
    return true;

  default:
    break;
  }

  return false;
}

/*
 * InfoBoxContentWind
 *
 * Subpart Panel Edit
 */

static int InfoBoxID;

Window*
InfoBoxContentWind::PnlEditLoad(SingleWindow &parent, TabBarControl* wTabBar,
                                WndForm* wf, const int id)
{
  assert(wTabBar);
  assert(wf);

  InfoBoxID = id;

  Window *wInfoBoxAccessEdit =
      LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_INFOBOXWINDEDIT"));
  assert(wInfoBoxAccessEdit);

  return wInfoBoxAccessEdit;
}

bool
InfoBoxContentWind::PnlEditOnTabPreShow(TabBarControl::EventType EventType)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  const SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.ExternalWind;

  WndProperty* wp;

  wp = (WndProperty*)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpSpeed"));
  if (wp) {
    wp->set_enabled(!external_wind);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetMax(Units::ToUserWindSpeed(Units::ToSysUnit(fixed(200), unKiloMeterPerHour)));
    df.SetUnits(Units::GetSpeedName());
    df.Set(Units::ToUserWindSpeed(CommonInterface::Calculated().wind.norm));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpDirection"));
  if (wp) {
    wp->set_enabled(!external_wind);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.Set(CommonInterface::Calculated().wind.bearing.Degrees());
    wp->RefreshDisplay();
  }

  return true;
}

void
InfoBoxContentWind::PnlEditOnWindSpeed(gcc_unused DataFieldFloat &Sender)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.ExternalWind;

  if (!external_wind) {
    settings_computer.ManualWind.norm =
      Units::ToSysWindSpeed(Sender.GetAsFixed());
    settings_computer.ManualWindAvailable.Update(basic.clock);
  }
}

void
InfoBoxContentWind::PnlEditOnWindDirection(gcc_unused DataFieldFloat &Sender)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.ExternalWind;

  if (!external_wind) {
    settings_computer.ManualWind.bearing = Angle::Degrees(Sender.GetAsFixed());
    settings_computer.ManualWindAvailable.Update(basic.clock);
  }
}

/*
 * Subpart Panel Setup
 */

Window*
InfoBoxContentWind::PnlSetupLoad(SingleWindow &parent, TabBarControl* wTabBar,
                                 WndForm* wf, const int id)
{
  assert(wTabBar);
  assert(wf);

  InfoBoxID = id;

  Window *wInfoBoxAccessSetup =
      LoadWindow(CallBackTable, wf, *wTabBar, _T("IDR_XML_INFOBOXWINDSETUP"));
  assert(wInfoBoxAccessSetup);

  const NMEAInfo &basic = XCSoarInterface::Basic();
  const SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.ExternalWind;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpAutoWind"));
  if (external_wind) {
    wp->set_enabled(false);
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    df.addEnumText(_("External"));
    df.Set(0);
    wp->RefreshDisplay();
  } else {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Manual"));
    dfe->addEnumText(_("Circling"));
    dfe->addEnumText(_("ZigZag"));
    dfe->addEnumText(_("Both"));
    dfe->Set(settings_computer.AutoWindMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpTrailDrift"));
  if (wp) {
    DataFieldBoolean &df = *(DataFieldBoolean *)wp->GetDataField();
    df.Set(XCSoarInterface::SettingsMap().EnableTrailDrift);
    wp->RefreshDisplay();
  }

  return wInfoBoxAccessSetup;
}

bool
InfoBoxContentWind::PnlSetupOnTabPreHide()
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.ExternalWind;

  if (!external_wind)
    SaveFormProperty(*dlgInfoBoxAccess::GetWindowForm(), _T("prpAutoWind"), szProfileAutoWind,
                     settings_computer.AutoWindMode);

  DataFieldEnum* dfe = (DataFieldEnum*)((WndProperty*)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAutoWind")))->GetDataField();

  if (_tcscmp(dfe->GetAsString(), _("Manual")) == 0)
    settings_computer.ManualWindAvailable.Update(basic.clock);

  SaveFormProperty(*dlgInfoBoxAccess::GetWindowForm(), _T("prpTrailDrift"),
                   XCSoarInterface::SetSettingsMap().EnableTrailDrift);
  ActionInterface::SendSettingsMap();

  return true;
}

void
InfoBoxContentWind::PnlSetupOnSetup(gcc_unused WndButton &Sender)
{
  InfoBoxManager::SetupFocused(InfoBoxID);
  dlgInfoBoxAccess::OnClose();
}

/*
 * Subpart callback function pointers
 */

static const InfoBoxContentWind::PanelContent Panels[] = {
InfoBoxContentWind::PanelContent (
  _("Edit"),
  (*InfoBoxContentWind::PnlEditLoad),
  NULL,
  (*InfoBoxContentWind::PnlEditOnTabPreShow)),

InfoBoxContentWind::PanelContent (
  _("Setup"),
  (*InfoBoxContentWind::PnlSetupLoad),
  (*InfoBoxContentWind::PnlSetupOnTabPreHide))
};

const InfoBoxContentWind::DialogContent InfoBoxContentWind::dlgContent = {
  ARRAY_SIZE(Panels), &Panels[0],
  &CallBackTable[0],
};

const InfoBoxContentWind::DialogContent *
InfoBoxContentWind::GetDialogContent() {
  return &dlgContent;
}


void
InfoBoxContentWindSpeed::Update(InfoBoxWindow &infobox)
{
  const DerivedInfo &info = CommonInterface::Calculated();
  if (!info.wind_available) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f"),
                    Units::ToUserWindSpeed(info.wind.norm));

  // Set Unit
  infobox.SetValueUnit(Units::Current.WindSpeedUnit);

  // Set Comment
  infobox.SetComment(info.wind.bearing, _T("T"));
}

void
InfoBoxContentWindBearing::Update(InfoBoxWindow &infobox)
{
  const DerivedInfo &info = CommonInterface::Calculated();
  if (!info.wind_available) {
    infobox.SetInvalid();
    return;
  }

  infobox.SetValue(info.wind.bearing, _T("T"));
}
