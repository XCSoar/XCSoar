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

#include "InfoBoxes/Content/Speed.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"

#include "Simulator.hpp"
#include "DeviceBlackboard.hpp"
#include "Message.hpp"
#include "Language.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentSpeedGround::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Basic().GroundSpeed,
                         tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::SpeedUnit);
}

bool
InfoBoxContentSpeedGround::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (!is_simulator())
    return false;

  fixed fixed_step = (fixed)Units::ToSysUnit(fixed_ten, Units::SpeedUnit);
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
  }

  return false;
}

void
InfoBoxContentSpeedIndicated::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Basic().AirspeedAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Basic().IndicatedAirspeed,
                         tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::SpeedUnit);
}

bool
InfoBoxContentSpeedIndicated::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    XCSoarInterface::SetSettingsComputer().EnableCalibration
        = !XCSoarInterface::SettingsComputer().EnableCalibration;

    Message::AddMessage(XCSoarInterface::SettingsComputer().EnableCalibration
                        ? _("Calibrate ON")
                        : _("Calibrate OFF"));
    return true;
  }

  return false;
}

void
InfoBoxContentSpeed::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Basic().AirspeedAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  Units::FormatUserSpeed(XCSoarInterface::Basic().TrueAirspeed,
                         tmp, 32, false);
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::SpeedUnit);
}

bool
InfoBoxContentSpeed::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    XCSoarInterface::SetSettingsComputer().EnableCalibration
        = !XCSoarInterface::SettingsComputer().EnableCalibration;

    Message::AddMessage(XCSoarInterface::SettingsComputer().EnableCalibration ?
                        _("Calibrate ON") :
                        _("Calibrate OFF"));
    return true;
  }

  return false;
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
  infobox.SetValueUnit(Units::SpeedUnit);
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
  infobox.SetValueUnit(Units::SpeedUnit);

  // Set Comment
  if (XCSoarInterface::SettingsComputer().EnableBlockSTF)
    infobox.SetComment(_("BLOCK"));
  else
    infobox.SetComment(_("DOLPHIN"));

}

