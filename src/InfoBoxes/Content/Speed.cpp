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

#include <tchar.h>

void
InfoBoxContentSpeedGround::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("V Gnd"));

  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"), (double)Units::ToUserSpeed(
      XCSoarInterface::Basic().GroundSpeed));
  infobox.SetValue(tmp);

  // Set Unit
  infobox.SetValueUnit(Units::SpeedUnit);
}

bool
InfoBoxContentSpeedGround::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (!is_simulator())
    return false;

  fixed fixed_step = (fixed)Units::ToSysUnit(10, Units::SpeedUnit);
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
  // Set Title
  infobox.SetTitle(_T("V IAS"));

  if (!XCSoarInterface::Basic().AirspeedAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"), (double)Units::ToUserSpeed(
      XCSoarInterface::Basic().IndicatedAirspeed));
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

    Message::AddMessage(XCSoarInterface::SettingsComputer().EnableCalibration ?
                        _T("Calibrate ON") :
                        _T("Calibrate OFF"));
    return true;
  }

  return false;
}

void
InfoBoxContentSpeed::Update(InfoBoxWindow &infobox)
{
  // Set Title
  infobox.SetTitle(_T("V TAS"));

  if (!XCSoarInterface::Basic().AirspeedAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  TCHAR tmp[32];
  _stprintf(tmp, _T("%2.0f"), (double)Units::ToUserSpeed(
      XCSoarInterface::Basic().TrueAirspeed));
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
                        _T("Calibrate ON") :
                        _T("Calibrate OFF"));
    return true;
  }

  return false;
}
