/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Renderer/ClimbPercentRenderer.hpp"

#include <tchar.h>

static void
SetVSpeed(InfoBoxData &data, double value) noexcept
{
  TCHAR buffer[32];
  FormatUserVerticalSpeed(value, buffer, false);
  data.SetValue(buffer[0] == _T('+') ? buffer + 1 : buffer);
  data.SetValueUnit(Units::current.vertical_speed_unit);
}

void
UpdateInfoBoxVario(InfoBoxData &data) noexcept
{
  SetVSpeed(data, CommonInterface::Basic().brutto_vario);
}

void
UpdateInfoBoxVarioNetto(InfoBoxData &data) noexcept
{
  SetVSpeed(data, CommonInterface::Basic().netto_vario);
}

void
UpdateInfoBoxThermal30s(InfoBoxData &data) noexcept
{
  SetVSpeed(data, CommonInterface::Calculated().average);

  // Set Color (red/black)
  data.SetValueColor(2 * CommonInterface::Calculated().average <
      CommonInterface::Calculated().common_stats.current_risk_mc ? 1 : 0);
}

void
UpdateInfoBoxThermalLastAvg(InfoBoxData &data) noexcept
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().last_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data, thermal.lift_rate);
}

void
UpdateInfoBoxThermalLastGain(InfoBoxData &data) noexcept
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().last_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(thermal.gain);
}

void
UpdateInfoBoxThermalLastTime(InfoBoxData &data) noexcept
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().last_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromTimeTwoLines(thermal.duration);
}

void
UpdateInfoBoxThermalAllAvg(InfoBoxData &data) noexcept
{
  if (CommonInterface::Calculated().time_circling.count() <= 0) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data, CommonInterface::Calculated().total_height_gain /
            CommonInterface::Calculated().time_circling.count());
}

void
UpdateInfoBoxThermalAvg(InfoBoxData &data) noexcept
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().current_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data, thermal.lift_rate);

  // Set Color (red/black)
  data.SetValueColor(thermal.lift_rate * 1.5 <
      CommonInterface::Calculated().common_stats.current_risk_mc ? 1 : 0);
}

void
UpdateInfoBoxThermalGain(InfoBoxData &data) noexcept
{
  const OneClimbInfo &thermal = CommonInterface::Calculated().current_thermal;
  if (!thermal.IsDefined()) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(thermal.gain);
}

void
UpdateInfoBoxThermalTime(InfoBoxData &data) noexcept
{
    const OneClimbInfo& thermal = CommonInterface::Calculated().current_thermal;
    if (!thermal.IsDefined()) {
        data.SetInvalid();
        return;
    }
    data.SetValueFromTimeTwoLines(thermal.duration);
}


void
UpdateInfoBoxThermalRatio(InfoBoxData &data) noexcept
{
  // Set Value

  if (CommonInterface::Calculated().circling_percentage < 0)
    data.SetInvalid();
  else {
    data.SetValueFromPercent(CommonInterface::Calculated().circling_percentage);
    data.SetCommentFromPercent(CommonInterface::Calculated().circling_climb_percentage);
  }
}

void
UpdateInfoBoxNonCirclingClimbRatio(InfoBoxData &data) noexcept
{
  // Set Value

  if (CommonInterface::Calculated().noncircling_climb_percentage < 0)
    data.SetInvalid();
  else
    data.SetValueFromPercent(CommonInterface::Calculated().noncircling_climb_percentage);
}

void
UpdateInfoBoxVarioDistance(InfoBoxData &data) noexcept
{
  if (!CommonInterface::Calculated().task_stats.task_valid) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data,
            CommonInterface::Calculated().task_stats.total.vario.get_value());

  // Set Color (red/black)
  data.SetValueColor(CommonInterface::Calculated().task_stats.total.vario.get_value() < 0 ? 1 : 0);
}


void
UpdateInfoBoxNextLegEqThermal(InfoBoxData &data) noexcept
{
  const auto next_leg_eq_thermal = CommonInterface::Calculated().next_leg_eq_thermal;
  if (next_leg_eq_thermal < 0) {
    data.SetInvalid();
    return;
  }

  SetVSpeed(data, next_leg_eq_thermal);
}

void
UpdateInfoBoxCircleDiameter(InfoBoxData &data) noexcept
{
  if (!CommonInterface::Basic().airspeed_available.IsValid()) {
    data.SetInvalid();
    return;
  }

  const Angle turn_rate =
    CommonInterface::Calculated().turn_rate_heading_smoothed.Absolute();

  // deal with div zero and small turn rates
  if (turn_rate < Angle::Degrees(1)) {
    data.SetInvalid();
    return;
  }

  const auto circle_diameter = CommonInterface::Basic().true_airspeed
     / turn_rate.Radians()
     * 2; // convert turn rate to radians/s and double it to get estimated circle diameter

  if (circle_diameter > 2000) { // arbitrary estimated that any diameter bigger than 2km will not be interesting
    data.SetInvalid();
    return;
  }

  TCHAR buffer[32];
  Unit unit = FormatSmallUserDistance(buffer, circle_diameter, false, 0);
  data.SetValue (buffer);
  data.SetValueUnit(unit);

  const auto circle_duration =
    Angle::FullCircle().Native() / turn_rate.Native();

  StaticString<16> duration_buffer;
  duration_buffer.Format(_T("%u s"), int(circle_duration));
  _tcscpy (buffer, duration_buffer);
  data.SetComment (buffer);
}

InfoBoxContentThermalAssistant::InfoBoxContentThermalAssistant() noexcept
  :renderer(UIGlobals::GetLook().thermal_assistant_gauge, 0, true) {}

void
InfoBoxContentThermalAssistant::Update(InfoBoxData &data) noexcept
{
  const auto &basic = CommonInterface::Basic();
  const auto &calculated = CommonInterface::Calculated();

  if (!calculated.circling) {
    data.SetInvalid();
    return;
  }

  data.SetCustom(basic.location_available.ToInteger() +
                 basic.track_available.ToInteger() +
                 basic.attitude.heading_available.ToInteger());

  renderer.Update(basic.attitude, calculated);
}

void
InfoBoxContentThermalAssistant::OnCustomPaint(Canvas &canvas,
                                              const PixelRect &rc)noexcept 
{
  renderer.UpdateLayout(rc);
  renderer.Paint(canvas);
}

void
InfoBoxContentClimbPercent::OnCustomPaint(Canvas &canvas, const PixelRect &rc)noexcept 
{
  const Look &look = UIGlobals::GetLook();
  ClimbPercentRenderer renderer(look.circling_percent);
  renderer.Draw(CommonInterface::Calculated(),
                canvas, rc,
                look.info_box.inverse);
}

void
InfoBoxContentClimbPercent::Update(InfoBoxData &data) noexcept
{
  const auto &basic = CommonInterface::Basic();

  // TODO: use an appropriate digest
  data.SetCustom(basic.location_available.ToInteger());
}
