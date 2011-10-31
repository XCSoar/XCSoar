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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "Blackboard.hpp"
#include "SettingsMap.hpp"
#include "SettingsComputer.hpp"
#include "Units/Units.hpp"
#include "Profile/Profile.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Float.hpp"
#include "DataField/Boolean.hpp"
#include "MainWindow.hpp"
#include "Engine/Navigation/SpeedVector.hpp"

static WndForm *wf = NULL;

static void
OnCancel(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrCancel);
}

static void
OnOK(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnOK),
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

  const bool external_wind = XCSoarInterface::Basic().external_wind_available &&
    XCSoarInterface::SettingsComputer().use_external_wind;

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
    df.Set(CommonInterface::Calculated().wind.bearing.Degrees());
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
    dfe->Set(XCSoarInterface::SettingsComputer().auto_wind_mode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpTrailDrift"));
  if (wp) {
    DataFieldBoolean &df = *(DataFieldBoolean *)wp->GetDataField();
    df.Set(XCSoarInterface::SettingsMap().trail_drift_enabled);
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
      XCSoarInterface::SetSettingsComputer().manual_wind.norm =
        Units::ToSysWindSpeed(df.GetAsFixed());
      XCSoarInterface::SetSettingsComputer().manual_wind_available.Update(XCSoarInterface::Basic().clock);
    }

    wp = (WndProperty*)wf->FindByName(_T("prpDirection"));
    if (wp != NULL) {
      DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
      XCSoarInterface::SetSettingsComputer().manual_wind.bearing =
        Angle::Degrees(df.GetAsFixed());
      XCSoarInterface::SetSettingsComputer().manual_wind_available.Update(XCSoarInterface::Basic().clock);
    }

    SaveFormProperty(*wf, _T("prpAutoWind"), szProfileAutoWind,
                     XCSoarInterface::SetSettingsComputer().auto_wind_mode);
  }

  SaveFormProperty(*wf, _T("prpTrailDrift"),
                   XCSoarInterface::SetSettingsMap().trail_drift_enabled);

  ActionInterface::SendSettingsMap();

  delete wf;
}
