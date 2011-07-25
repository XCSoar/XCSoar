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

#include "InfoBoxes/Content/Thermal.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Units/UnitsFormatter.hpp"
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
  infobox.SetValueUnit(Units::Current.VerticalSpeedUnit);
}

void
InfoBoxContentVario::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, CommonInterface::Basic().BruttoVario);
}

void
InfoBoxContentVarioNetto::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, CommonInterface::Basic().NettoVario);
}

void
InfoBoxContentThermal30s::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Calculated().average);

  // Set Color (red/black)
  infobox.SetColor(XCSoarInterface::Calculated().average * fixed_two <
      XCSoarInterface::Calculated().common_stats.current_risk_mc ? 1 : 0);
}

void
InfoBoxContentThermalLastAvg::Update(InfoBoxWindow &infobox)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().last_thermal;
  if (!thermal.IsDefined()) {
    infobox.SetInvalid();
    return;
  }

  SetVSpeed(infobox, thermal.lift_rate);
}

void
InfoBoxContentThermalLastGain::Update(InfoBoxWindow &infobox)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().last_thermal;
  if (!thermal.IsDefined()) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR sTmp[32];
  Units::FormatUserAltitude(thermal.gain, sTmp, 32, false);
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
}

void
InfoBoxContentThermalLastTime::Update(InfoBoxWindow &infobox)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().last_thermal;
  if (!thermal.IsDefined()) {
    infobox.SetInvalid();
    return;
  }

  // Set Value

  TCHAR value[32], comment[32];
  Units::TimeToTextSmart(value, comment, (int)thermal.duration);

  infobox.SetValue(value);
  infobox.SetComment(comment);
}

void
InfoBoxContentThermalAllAvg::Update(InfoBoxWindow &infobox)
{
  if (!positive(XCSoarInterface::Calculated().time_climb)) {
    infobox.SetInvalid();
    return;
  }

  SetVSpeed(infobox, XCSoarInterface::Calculated().total_height_gain /
            XCSoarInterface::Calculated().time_climb);
}

void
InfoBoxContentThermalAvg::Update(InfoBoxWindow &infobox)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().current_thermal;
  if (!thermal.IsDefined()) {
    infobox.SetInvalid();
    return;
  }

  SetVSpeed(infobox, thermal.lift_rate);

  // Set Color (red/black)
  infobox.SetColor(thermal.lift_rate * fixed(1.5) <
      XCSoarInterface::Calculated().common_stats.current_risk_mc ? 1 : 0);
}

void
InfoBoxContentThermalGain::Update(InfoBoxWindow &infobox)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().current_thermal;
  if (!thermal.IsDefined()) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR sTmp[32];
  Units::FormatUserAltitude(thermal.gain, sTmp, 32, false);
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
}

void
InfoBoxContentThermalRatio::Update(InfoBoxWindow &infobox)
{
  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f%%"),
                    XCSoarInterface::Calculated().circling_percentage);
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

  // Set Color (red/black)
  infobox.SetColor(negative(
      XCSoarInterface::Calculated().task_stats.total.vario.get_value()) ? 1 : 0);
}
