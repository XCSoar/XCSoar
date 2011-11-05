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
#include "InfoBoxes/Panel/WindEdit.hpp"
#include "InfoBoxes/Panel/WindSetup.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Util/Macros.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentHumidity::Update(InfoBoxData &data)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  if (!basic.humidity_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%d"), (int)basic.humidity);
  data.SetValue(tmp);
}

void
InfoBoxContentTemperature::Update(InfoBoxData &data)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  if (!basic.temperature_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(data, _T("%2.1f")_T(DEG),
                    Units::ToUserTemperature(basic.temperature));
}

void
InfoBoxContentTemperatureForecast::Update(InfoBoxData &data)
{
  fixed temperature = CommonInterface::SettingsComputer().forecast_temperature;
  SetValueFromFixed(data, _T("%2.1f")_T(DEG),
                    Units::ToUserTemperature(temperature));
}

bool
InfoBoxContentTemperatureForecast::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch(keycode) {
  case ibkUp:
    CommonInterface::SetSettingsComputer().forecast_temperature += fixed_half;
    return true;

  case ibkDown:
    CommonInterface::SetSettingsComputer().forecast_temperature -= fixed_half;
    return true;

  default:
    break;
  }

  return false;
}

/*
 * Subpart callback function pointers
 */

static gcc_constexpr_data InfoBoxContentWind::PanelContent Panels[] = {
InfoBoxContentWind::PanelContent (
  N_("Edit"),
  LoadWindEditPanel,
  NULL,
  WindEditPanelPreShow),

InfoBoxContentWind::PanelContent (
  N_("Setup"),
  LoadWindSetupPanel,
  WindSetupPanelPreHide),
};

const InfoBoxContentWind::DialogContent InfoBoxContentWind::dlgContent = {
  ARRAY_SIZE(Panels), &Panels[0],
};

const InfoBoxContentWind::DialogContent *
InfoBoxContentWind::GetDialogContent() {
  return &dlgContent;
}


void
InfoBoxContentWindSpeed::Update(InfoBoxData &data)
{
  const DerivedInfo &info = CommonInterface::Calculated();
  if (!info.wind_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(data, _T("%2.0f"),
                    Units::ToUserWindSpeed(info.wind.norm));

  // Set Unit
  data.SetValueUnit(Units::current.wind_speed_unit);

  // Set Comment
  data.SetComment(info.wind.bearing, _T("T"));
}

void
InfoBoxContentWindBearing::Update(InfoBoxData &data)
{
  const DerivedInfo &info = CommonInterface::Calculated();
  if (!info.wind_available) {
    data.SetInvalid();
    return;
  }

  data.SetValue(info.wind.bearing, _T("T"));
}
