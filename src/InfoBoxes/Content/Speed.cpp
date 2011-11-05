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

#include "InfoBoxes/Content/Speed.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"

#include "Simulator.hpp"
#include "DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Language/Language.hpp"
#include "Units/UnitsFormatter.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentSpeedGround::Update(InfoBoxData &data)
{

  if (!XCSoarInterface::Basic().ground_speed_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Basic().ground_speed,
                         tmp, 32, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.speed_unit);
}

bool
InfoBoxContentSpeedGround::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (!is_simulator())
    return false;
  if (!XCSoarInterface::Basic().gps.simulator)
    return false;

  fixed fixed_step = (fixed)Units::ToSysSpeed(fixed_ten);
  const Angle a5 = Angle::Degrees(fixed(5));

  switch (keycode) {
  case ibkUp:
    device_blackboard->SetSpeed(
        XCSoarInterface::Basic().ground_speed + fixed_step);
    return true;

  case ibkDown:
    device_blackboard->SetSpeed(
        max(fixed_zero, XCSoarInterface::Basic().ground_speed - fixed_step));
    return true;

  case ibkLeft:
    device_blackboard->SetTrack(XCSoarInterface::Basic().track - a5);
    return true;

  case ibkRight:
    device_blackboard->SetTrack(XCSoarInterface::Basic().track + a5);
    return true;

  case ibkEnter:
    break;
  }

  return false;
}

void
InfoBoxContentSpeedIndicated::Update(InfoBoxData &data)
{
  if (!XCSoarInterface::Basic().airspeed_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Basic().indicated_airspeed,
                         tmp, 32, false, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.speed_unit);
}

void
InfoBoxContentSpeed::Update(InfoBoxData &data)
{
  if (!XCSoarInterface::Basic().airspeed_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Basic().true_airspeed,
                         tmp, 32, false, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.speed_unit);
}

void
InfoBoxContentSpeedMacCready::Update(InfoBoxData &data)
{
  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Calculated().common_stats.V_block,
                         tmp, 32, false, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.speed_unit);
}

void
InfoBoxContentSpeedDolphin::Update(InfoBoxData &data)
{
  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Calculated().V_stf,
                         tmp, 32, false, false);
  data.SetValue(tmp);

  // Set Unit
  data.SetValueUnit(Units::current.speed_unit);

  // Set Comment
  if (XCSoarInterface::SettingsComputer().block_stf_enabled)
    data.SetComment(_("BLOCK"));
  else
    data.SetComment(_("DOLPHIN"));

}

