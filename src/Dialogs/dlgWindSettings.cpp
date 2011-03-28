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

#include "Dialogs/Internal.hpp"
#include "Blackboard.hpp"
#include "SettingsMap.hpp"
#include "SettingsComputer.hpp"
#include "Units.hpp"
#include "Profile/Profile.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "DataField/Boolean.hpp"
#include "MainWindow.hpp"
#include "Engine/Navigation/SpeedVector.hpp"

static WndForm *wf = NULL;

static void
OnCancel(WndButton &Sender)
{
  (void)Sender;
  wf->SetModalResult(mrCancel);
}

static void
OnOkay(WndButton &Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnOkay),
  DeclareCallBackEntry(OnCancel),
  DeclareCallBackEntry(NULL)
};

void
dlgWindSettingsShowModal(void)
{
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
		                  _T("IDR_XML_WINDSETTINGS"));
  if (wf == NULL)
    return;

  const bool external_wind = XCSoarInterface::Basic().ExternalWindAvailable &&
    XCSoarInterface::SettingsComputer().ExternalWind;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpSpeed"));
  if (wp) {
    wp->set_enabled(!external_wind);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetMax(Units::ToUserWindSpeed(Units::ToSysUnit(fixed(200), unKiloMeterPerHour)));
    df.SetUnits(Units::GetSpeedName());
    df.Set(Units::ToUserWindSpeed(CommonInterface::Calculated().wind.norm));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDirection"));
  if (wp) {
    wp->set_enabled(!external_wind);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.Set(CommonInterface::Calculated().wind.bearing.value_degrees());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAutoWind"));
  assert(wp != NULL);
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
    dfe->Set(XCSoarInterface::SettingsComputer().AutoWindMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTrailDrift"));
  if (wp) {
    DataFieldBoolean &df = *(DataFieldBoolean *)wp->GetDataField();
    df.Set(XCSoarInterface::SettingsMap().EnableTrailDrift);
    wp->RefreshDisplay();
  }

  if (wf->ShowModal() != mrOK) {
    delete wf;
    return;
  }

  if (!external_wind) {
    wp = (WndProperty*)wf->FindByName(_T("prpSpeed"));
    if (wp != NULL) {
      DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
      XCSoarInterface::SetSettingsComputer().ManualWind.norm =
        Units::ToSysWindSpeed(df.GetAsFixed());
      XCSoarInterface::SetSettingsComputer().ManualWindAvailable.update(XCSoarInterface::Basic().Time);
    }

    wp = (WndProperty*)wf->FindByName(_T("prpDirection"));
    if (wp != NULL) {
      DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
      XCSoarInterface::SetSettingsComputer().ManualWind.bearing =
        Angle::degrees(df.GetAsFixed());
      XCSoarInterface::SetSettingsComputer().ManualWindAvailable.update(XCSoarInterface::Basic().Time);
    }

    SaveFormProperty(*wf, _T("prpAutoWind"), szProfileAutoWind,
                     XCSoarInterface::SetSettingsComputer().AutoWindMode);
  }

  SaveFormProperty(*wf, _T("prpTrailDrift"),
                   XCSoarInterface::SetSettingsMap().EnableTrailDrift);

  ActionInterface::SendSettingsMap();

  delete wf;
}
