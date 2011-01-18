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

#include "InfoBoxes/Content/Altitude.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Units.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#include "DeviceBlackboard.hpp"
#include "Simulator.hpp"

#include <tchar.h>

void
InfoBoxContentAltitudeGPS::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  if (XCSoarInterface::Basic().gps.NAVWarning){
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Units::FormatUserAltitude(XCSoarInterface::Basic().GPSAltitude, sTmp,
                            sizeof(sTmp) / sizeof(sTmp[0]), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(XCSoarInterface::Basic().GPSAltitude, sTmp,
                                     sizeof(sTmp) / sizeof(sTmp[0]));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

bool
InfoBoxContentAltitudeGPS::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (!is_simulator())
    return false;
  if (XCSoarInterface::Basic().gps.Replay)
    return false;

  fixed fixed_step = (fixed)Units::ToSysUnit(fixed(100), Units::AltitudeUnit);
  const Angle a5 = Angle::degrees(fixed(5));

  switch (keycode) {
  case ibkUp:
    device_blackboard.SetAltitude(
        XCSoarInterface::Basic().GPSAltitude + fixed_step);
    return true;

  case ibkDown:
    device_blackboard.SetAltitude(
        max(fixed_zero, XCSoarInterface::Basic().GPSAltitude - fixed_step));
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
InfoBoxContentAltitudeAGL::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  if (XCSoarInterface::Basic().gps.NAVWarning
      || !XCSoarInterface::Calculated().TerrainValid) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Units::FormatUserAltitude(XCSoarInterface::Basic().AltitudeAGL, sTmp,
                            sizeof(sTmp) / sizeof(sTmp[0]), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(XCSoarInterface::Basic().AltitudeAGL, sTmp,
                                     sizeof(sTmp) / sizeof(sTmp[0]));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);

  // Set Color (red/black)
  infobox.SetColor(XCSoarInterface::Basic().AltitudeAGL <
      XCSoarInterface::SettingsComputer().safety_height_terrain ? 1 : 0);
}

void
InfoBoxContentAltitudeBaro::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  if (!XCSoarInterface::Basic().BaroAltitudeAvailable) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Units::FormatUserAltitude(XCSoarInterface::Basic().BaroAltitude, sTmp,
                            sizeof(sTmp) / sizeof(sTmp[0]), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(XCSoarInterface::Basic().BaroAltitude, sTmp,
                                     sizeof(sTmp) / sizeof(sTmp[0]));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentAltitudeQFE::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  fixed Value = XCSoarInterface::Basic().GPSAltitude;

  const Waypoint* home_waypoint = way_points.find_home();
  if (home_waypoint)
    Value -= home_waypoint->Altitude;

  // Set Value
  Units::FormatUserAltitude(Value, sTmp,
                            sizeof(sTmp) / sizeof(sTmp[0]), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(Value, sTmp,
                                     sizeof(sTmp) / sizeof(sTmp[0]));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}

void
InfoBoxContentTerrainHeight::Update(InfoBoxWindow &infobox)
{
  TCHAR sTmp[32];

  if (XCSoarInterface::Basic().gps.NAVWarning
      || !XCSoarInterface::Calculated().TerrainValid){
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Units::FormatUserAltitude(XCSoarInterface::Calculated().TerrainAlt, sTmp,
                            sizeof(sTmp) / sizeof(sTmp[0]), false);
  infobox.SetValue(sTmp);

  // Set Comment
  Units::FormatAlternateUserAltitude(XCSoarInterface::Calculated().TerrainAlt, sTmp,
                                     sizeof(sTmp) / sizeof(sTmp[0]));
  infobox.SetComment(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::AltitudeUnit);
}
