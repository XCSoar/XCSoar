/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "InfoBoxes/Data.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

#include <tchar.h>
#include <stdio.h>

static void
SetVSpeed(InfoBoxData &data, fixed value)
{
  TCHAR buffer[32];
  FormatUserVerticalSpeed(value, buffer, false);
  data.SetValue(buffer[0] == _T('+') ? buffer + 1 : buffer);
  data.SetValueUnit(Units::current.vertical_speed_unit);
}

void
InfoBoxContentVario::Update(InfoBoxData &data)
{
  SetVSpeed(data, CommonInterface::Basic().brutto_vario);
}

void
InfoBoxContentVarioNetto::Update(InfoBoxData &data)
{
  SetVSpeed(data, CommonInterface::Basic().netto_vario);
}

void
InfoBoxContentThermal30s::Update(InfoBoxData &data)
{
  SetVSpeed(data, XCSoarInterface::Calculated().average);

  // Set Color (red/black)
  data.SetValueColor(Double(CommonInterface::Calculated().average) <
      XCSoarInterface::Calculated().common_stats.current_risk_mc ? 1 : 0);
}

void
InfoBoxContentThermalLastAvg::Update(InfoBoxData &data)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().last_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data, thermal.lift_rate);
}

void
InfoBoxContentThermalLastGain::Update(InfoBoxData &data)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().last_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(thermal.gain);
}

void
InfoBoxContentThermalLastTime::Update(InfoBoxData &data)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().last_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  // Set Value

  TCHAR value[32], comment[32];
  FormatTimeTwoLines(value, comment, (int)thermal.duration);

  data.SetValue(value);
  data.SetComment(comment);
}

void
InfoBoxContentThermalAllAvg::Update(InfoBoxData &data)
{
  if (!positive(XCSoarInterface::Calculated().time_climb)) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data, XCSoarInterface::Calculated().total_height_gain /
            XCSoarInterface::Calculated().time_climb);
}

void
InfoBoxContentThermalAvg::Update(InfoBoxData &data)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().current_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data, thermal.lift_rate);

  // Set Color (red/black)
  data.SetValueColor(thermal.lift_rate * fixed(1.5) <
      XCSoarInterface::Calculated().common_stats.current_risk_mc ? 1 : 0);
}

void
InfoBoxContentThermalGain::Update(InfoBoxData &data)
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().current_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(thermal.gain);
}

void
InfoBoxContentThermalRatio::Update(InfoBoxData &data)
{
  // Set Value

  if (negative(XCSoarInterface::Calculated().circling_percentage))
    data.SetInvalid();
  else
    data.SetValue(_T("%2.0f%%"),
                  XCSoarInterface::Calculated().circling_percentage);
}

void
InfoBoxContentVarioDistance::Update(InfoBoxData &data)
{
  if (!XCSoarInterface::Calculated().task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data,
            XCSoarInterface::Calculated().task_stats.total.vario.get_value());

  // Set Color (red/black)
  data.SetValueColor(negative(
      XCSoarInterface::Calculated().task_stats.total.vario.get_value()) ? 1 : 0);
}


void
InfoBoxContentNextLegEqThermal::Update(InfoBoxData &data)
{
  const fixed next_leg_eq_thermal = CommonInterface::Calculated().next_leg_eq_thermal;
  if (negative(next_leg_eq_thermal)) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data, next_leg_eq_thermal);
}

InfoBoxContentThermalAssistant::InfoBoxContentThermalAssistant()
  :renderer(UIGlobals::GetLook().thermal_assistant_gauge, 0, true) {}

void
InfoBoxContentThermalAssistant::Update(InfoBoxData &data)
{
  if (!XCSoarInterface::Calculated().circling) {
    data.SetInvalid();
    return;
  }

  data.SetCustom();

  renderer.Update(XCSoarInterface::Basic().attitude,
                  XCSoarInterface::Calculated());
}

void
InfoBoxContentThermalAssistant::OnCustomPaint(Canvas &canvas,
                                              const PixelRect &rc)
{
  renderer.Paint(canvas, rc);
}
