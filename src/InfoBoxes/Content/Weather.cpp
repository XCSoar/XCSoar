/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.1f")_T(DEG),
            (double)Units::ToUserTemperature(
                XCSoarInterface::Basic().OutsideAirTemperature));
  infobox.SetValue(tmp);
}

void
InfoBoxContentTemperatureForecast::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.1f")_T(DEG), CuSonde::maxGroundTemperature);
  infobox.SetValue(tmp);
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
  }

  return false;
}

void
InfoBoxContentWindSpeed::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"),
            (double)Units::ToUserWindSpeed(XCSoarInterface::Basic().wind.norm));
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::WindSpeedUnit);

  // Set Comment
  _stprintf(tmp, _T("%2.0f")_T(DEG)_T("T"),
            (double)XCSoarInterface::Basic().wind.bearing.value_degrees());
  infobox.SetComment(tmp);
}

void
InfoBoxContentWindBearing::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f")_T(DEG)_T("T"),
            (double)XCSoarInterface::Basic().wind.bearing.value_degrees());
  infobox.SetValue(tmp);
}
