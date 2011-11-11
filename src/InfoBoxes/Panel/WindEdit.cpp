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

#include "WindEdit.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/TabBar.hpp"
#include "DataField/Float.hpp"

static void
PnlEditOnWindSpeed(gcc_unused DataFieldFloat &Sender)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.use_external_wind;

  if (!external_wind) {
    settings_computer.manual_wind.norm =
      Units::ToSysWindSpeed(Sender.GetAsFixed());
    settings_computer.manual_wind_available.Update(basic.clock);
  }
}

static void
PnlEditOnWindDirection(gcc_unused DataFieldFloat &Sender)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SetSettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.use_external_wind;

  if (!external_wind) {
    settings_computer.manual_wind.bearing = Angle::Degrees(Sender.GetAsFixed());
    settings_computer.manual_wind_available.Update(basic.clock);
  }
}

static gcc_constexpr_data
CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(PnlEditOnWindSpeed),
  DeclareCallBackEntry(PnlEditOnWindDirection),
  DeclareCallBackEntry(NULL)
};

Window *
LoadWindEditPanel(SingleWindow &parent, TabBarControl *wTabBar,
                  WndForm *wf, const int id)
{
  assert(wTabBar);
  assert(wf);

  Window *wInfoBoxAccessEdit =
    LoadWindow(CallBackTable, wf, wTabBar->GetClientAreaWindow(),
               _T("IDR_XML_INFOBOXWINDEDIT"));
  assert(wInfoBoxAccessEdit);

  return wInfoBoxAccessEdit;
}

bool
WindEditPanelPreShow()
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  const SETTINGS_COMPUTER &settings_computer =
    XCSoarInterface::SettingsComputer();
  const bool external_wind = basic.external_wind_available &&
    settings_computer.use_external_wind;

  WndProperty* wp;

  const SpeedVector wind = CommonInterface::Calculated().GetWindOrZero();

  wp = (WndProperty*)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpSpeed"));
  if (wp) {
    wp->set_enabled(!external_wind);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetMax(Units::ToUserWindSpeed(Units::ToSysUnit(fixed(200), unKiloMeterPerHour)));
    df.SetUnits(Units::GetSpeedName());
    df.Set(Units::ToUserWindSpeed(wind.norm));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpDirection"));
  if (wp) {
    wp->set_enabled(!external_wind);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.Set(wind.bearing.Degrees());
    wp->RefreshDisplay();
  }

  return true;
}
