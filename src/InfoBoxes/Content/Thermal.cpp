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

void
InfoBoxContentMacCready::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  // Set Title
  infobox.SetTitle(_T("MacCready"));

  // Set Value
  _stprintf(sTmp, _T("%2.1f"), Units::ToUserVSpeed(
      XCSoarInterface::Calculated().common_stats.current_risk_mc));
  infobox.SetValue(sTmp);

  // Set Comment
  if (XCSoarInterface::SettingsComputer().auto_mc)
    infobox.SetComment(_T("AUTO"));
  else
    infobox.SetComment(_T("MANUAL"));

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);
}

bool
InfoBoxContentMacCready::HandleKey(const InfoBoxKeyCodes keycode)
{
  ProtectedTaskManager::ExclusiveLease task_manager(protected_task_manager);
  GlidePolar polar = task_manager->get_glide_polar();
  double mc = polar.get_mc();

  switch (keycode) {
  case ibkUp:
    mc = std::min(mc + (double)0.1, 5.0);
    polar.set_mc(fixed(mc));
    task_manager->set_glide_polar(polar);
    device_blackboard.SetMC(fixed(mc));
    return true;

  case ibkDown:
    mc = std::max(mc - (double)0.1, 0.0);
    polar.set_mc(fixed(mc));
    task_manager->set_glide_polar(polar);
    device_blackboard.SetMC(fixed(mc));
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
  TCHAR sTmp[32];

  // Set Title
  infobox.SetTitle(_T("Vario"));

  // Set Value
  _stprintf(sTmp, _T("%-2.1f"), Units::ToUserVSpeed(
            XCSoarInterface::Basic().TotalEnergyVario));
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);
}

void
InfoBoxContentVarioNetto::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  // Set Title
  infobox.SetTitle(_T("Netto"));

  // Set Value
  _stprintf(sTmp, _T("%-2.1f"), Units::ToUserVSpeed(
            XCSoarInterface::Basic().NettoVario));
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);
}

void
InfoBoxContentThermal30s::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  // Set Title
  infobox.SetTitle(_T("TC 30s"));

  // Set Value
  _stprintf(sTmp, _T("%-2.1f"), Units::ToUserVSpeed(
            XCSoarInterface::Calculated().Average30s));
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);

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
  // Set Title
  infobox.SetTitle(_T("TL Avg"));

  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%-2.1f"), Units::ToUserVSpeed(
      XCSoarInterface::Calculated().LastThermalAverage));
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);
}

void
InfoBoxContentThermalLastGain::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("TL Gain"));

  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%2.0f"), (double)Units::ToUserAltitude(
      XCSoarInterface::Calculated().LastThermalGain));
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentThermalLastTime::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("TL Time"));

  // Set Value
  TCHAR sTmp[32];
  int dd = (int)abs(XCSoarInterface::Calculated().LastThermalTime) % (3600 * 24);
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
  // Set Title
  infobox.SetTitle(_T("TC All"));

  if (!positive(XCSoarInterface::Calculated().timeCircling)) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%-2.1f"), Units::ToUserVSpeed(
      XCSoarInterface::Calculated().TotalHeightClimb /
      XCSoarInterface::Calculated().timeCircling));
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);
}

void
InfoBoxContentThermalAvg::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("TC Avg"));

  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%-2.1f"), Units::ToUserVSpeed(
      XCSoarInterface::Calculated().AverageThermal));
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);

  if (XCSoarInterface::Calculated().AverageThermal <
      fixed(0.667) * XCSoarInterface::Calculated().common_stats.current_risk_mc)
    // red
    infobox.SetColor(1);
  else
    infobox.SetColor(0);
}

void
InfoBoxContentThermalGain::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("TC Gain"));

  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%2.0f"), (double)Units::ToUserAltitude(
      XCSoarInterface::Calculated().ThermalGain));
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentThermalRatio::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("% Climb"));

  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%2.0f%%"),
            (double)XCSoarInterface::Calculated().PercentCircling);
  infobox.SetValue(sTmp);
}

void
InfoBoxContentVarioDistance::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("D Vario"));

  if (!XCSoarInterface::Calculated().task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR sTmp[32];
  _stprintf(sTmp, _T("%-2.1f"), Units::ToUserVSpeed(
      XCSoarInterface::Calculated().task_stats.total.vario.get_value()));
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::VerticalSpeedUnit);

  // Set Color
  if (XCSoarInterface::Calculated().task_stats.total.vario.get_value() < 0)
    // Red
    infobox.SetColor(1);
  else
    // Black
    infobox.SetColor(0);
}
