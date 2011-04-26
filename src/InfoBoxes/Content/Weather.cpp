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
#include "Interface.hpp"
#include "Atmosphere.h"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentHumidity::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Basic().HumidityAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%d"), (int)XCSoarInterface::Basic().RelativeHumidity);
  infobox.SetValue(tmp);
}

void
InfoBoxContentTemperature::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Basic().TemperatureAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.1f")_T(DEG),
      Units::ToUserTemperature(XCSoarInterface::Basic().OutsideAirTemperature));
}

void
InfoBoxContentTemperatureForecast::Update(InfoBoxWindow &infobox)
{
  // Set Value
  SetValueFromFixed(infobox, _T("%2.1f")_T(DEG),
                    fixed(CuSonde::maxGroundTemperature));
}

bool
InfoBoxContentTemperatureForecast::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch(keycode) {
  case ibkUp:
    CuSonde::adjustForecastTemperature(0.5);
    return true;

  case ibkDown:
    CuSonde::adjustForecastTemperature(-0.5);
    return true;

  default:
    break;
  }

  return false;
}

void
InfoBoxContentWindSpeed::Update(InfoBoxWindow &infobox)
{
  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f"),
                    Units::ToUserWindSpeed(XCSoarInterface::Basic().wind.norm));

  // Set Unit
  infobox.SetValueUnit(Units::WindSpeedUnit);

  // Set Comment
  infobox.SetComment(XCSoarInterface::Basic().wind.bearing, _T("T"));
}

void
InfoBoxContentWindBearing::Update(InfoBoxWindow &infobox)
{
  infobox.SetValue(XCSoarInterface::Basic().wind.bearing, _T("T"));
}
