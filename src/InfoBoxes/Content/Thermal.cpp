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

#include "InfoBoxes/Content/Thermal.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Units.hpp"
#include "Interface.hpp"

#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "DeviceBlackboard.hpp"

#include <tchar.h>
#include <stdio.h>

static void
SetVSpeed(InfoBoxWindow &infobox, fixed value)
{
  TCHAR buffer[32];
  Units::FormatUserVSpeed(value, buffer, 32, false);
  infobox.SetValue(buffer[0] == _T('+') ? buffer + 1 : buffer);
  infobox.SetValueUnit(Units::VerticalSpeedUnit);
}

void
InfoBoxContentMacCready::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox,
            XCSoarInterface::Calculated().common_stats.current_risk_mc);

  // Set Comment
  if (XCSoarInterface::SettingsComputer().auto_mc)
    infobox.SetComment(_("AUTO"));
  else
    infobox.SetComment(_("MANUAL"));
}

bool
InfoBoxContentMacCready::HandleKey(const InfoBoxKeyCodes keycode)
{
  GlidePolar polar = protected_task_manager.get_glide_polar();
  fixed mc = polar.get_mc();

  switch (keycode) {
  case ibkUp:
    mc = std::min(mc + fixed_one / 10, fixed(5));
    polar.set_mc(mc);
    protected_task_manager.set_glide_polar(polar);
    device_blackboard.SetMC(mc);
    return true;

  case ibkDown:
    mc = std::max(mc - fixed_one / 10, fixed_zero);
    polar.set_mc(mc);
    protected_task_manager.set_glide_polar(polar);
    device_blackboard.SetMC(mc);
    return true;

  case ibkLeft:
    XCSoarInterface::SetSettingsComputer().auto_mc = false;
    return true;

  case ibkRight:
    XCSoarInterface::SetSettingsComputer().auto_mc = true;
    return true;

  case ibkEnter:
    XCSoarInterface::SetSettingsComputer().auto_mc =
        !XCSoarInterface::SettingsComputer().auto_mc;
    return true;
  }
  return false;
}

void
InfoBoxContentVario::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Basic().TotalEnergyVario);
}

void
InfoBoxContentVarioNetto::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Basic().NettoVario);
}

void
InfoBoxContentThermal30s::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Calculated().Average30s);

  if (XCSoarInterface::Calculated().Average30s <
      fixed_half * XCSoarInterface::Calculated().common_stats.current_risk_mc)
    // red
    infobox.SetColor(1);
  else
    infobox.SetColor(0);
}

void
InfoBoxContentThermalLastAvg::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Calculated().LastThermalAverage);
}

void
InfoBoxContentThermalLastGain::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR sTmp[32];
  Units::FormatUserAltitude(XCSoarInterface::Calculated().LastThermalGain,
                            sTmp, 32, false);
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentThermalLastTime::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR sTmp[32];
  int dd = abs((int)XCSoarInterface::Calculated().LastThermalTime) % (3600 * 24);
  int hours = dd / 3600;
  int mins = dd / 60 - hours * 60;
  int seconds = dd - mins * 60 - hours * 3600;

  if (hours > 0) { // hh:mm, ss
    _stprintf(sTmp, _T("%02d:%02d"), hours, mins);
    infobox.SetValue(sTmp);
    _stprintf(sTmp, _T("%02d"), seconds);
    infobox.SetComment(sTmp);
  } else { // mm:ss
    _stprintf(sTmp, _T("%02d:%02d"), mins, seconds);
    infobox.SetValue(sTmp);
    infobox.SetComment(_T(""));
  }
}

void
InfoBoxContentThermalAllAvg::Update(InfoBoxWindow &infobox)
{
  if (!positive(XCSoarInterface::Calculated().timeCircling)) {
    infobox.SetInvalid();
    return;
  }

  SetVSpeed(infobox, XCSoarInterface::Calculated().TotalHeightClimb /
            XCSoarInterface::Calculated().timeCircling);
}

void
InfoBoxContentThermalAvg::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Calculated().ThermalAverage);

  if (XCSoarInterface::Calculated().ThermalAverage <
      fixed_two_thirds * XCSoarInterface::Calculated().common_stats.current_risk_mc)
    // red
    infobox.SetColor(1);
  else
    infobox.SetColor(0);
}

void
InfoBoxContentThermalGain::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR sTmp[32];
  Units::FormatUserAltitude(XCSoarInterface::Calculated().ThermalGain,
                            sTmp, 32, false);
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentThermalRatio::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%2.0f%%"),
            (double)XCSoarInterface::Calculated().PercentCircling);
  infobox.SetValue(sTmp);
}

void
InfoBoxContentVarioDistance::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Calculated().task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  SetVSpeed(infobox,
            XCSoarInterface::Calculated().task_stats.total.vario.get_value());

  // Set Color
  if (negative(XCSoarInterface::Calculated().task_stats.total.vario.get_value()))
    // Red
    infobox.SetColor(1);
  else
    // Black
    infobox.SetColor(0);
}
