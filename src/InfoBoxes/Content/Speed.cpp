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

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"

#include "Simulator.hpp"
#include "DeviceBlackboard.hpp"
#include "Message.hpp"
#include "Language.hpp"
#include "Units/UnitsFormatter.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentSpeedGround::Update(InfoBoxWindow &infobox)
{

  if (!XCSoarInterface::Basic().GroundSpeedAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Basic().GroundSpeed,
                         tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.SpeedUnit);
}

bool
InfoBoxContentSpeedGround::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (!is_simulator())
    return false;
  if (!XCSoarInterface::Basic().gps.Simulator)
    return false;

  fixed fixed_step = (fixed)Units::ToSysSpeed(fixed_ten);
  const Angle a5 = Angle::degrees(fixed(5));

  switch (keycode) {
  case ibkUp:
    device_blackboard.SetSpeed(
        XCSoarInterface::Basic().GroundSpeed + fixed_step);
    return true;

  case ibkDown:
    device_blackboard.SetSpeed(
        max(fixed_zero, XCSoarInterface::Basic().GroundSpeed - fixed_step));
    return true;

  case ibkLeft:
    device_blackboard.SetTrackBearing(
        XCSoarInterface::Basic().TrackBearing - a5);
    return true;

  case ibkRight:
    device_blackboard.SetTrackBearing(
        XCSoarInterface::Basic().TrackBearing + a5);
    return true;

  case ibkEnter:
    break;
  }

  return false;
}

void
InfoBoxContentSpeedIndicated::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Calculated().AirspeedAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Calculated().IndicatedAirspeed,
                         tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.SpeedUnit);
}

void
InfoBoxContentSpeed::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Calculated().AirspeedAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Calculated().TrueAirspeed,
                         tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.SpeedUnit);
}

void
InfoBoxContentSpeedMacCready::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Calculated().common_stats.V_block,
                         tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.SpeedUnit);
}

void
InfoBoxContentSpeedDolphin::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Calculated().V_stf,
                         tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.SpeedUnit);

  // Set Comment
  if (XCSoarInterface::SettingsComputer().EnableBlockSTF)
    infobox.SetComment(_("BLOCK"));
  else
    infobox.SetComment(_("DOLPHIN"));

}

