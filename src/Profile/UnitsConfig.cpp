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

#include "Profile/UnitsConfig.hpp"

#include "Profile/Profile.hpp"
#include "Units/Units.hpp"
#include "Units/UnitsGlue.hpp"

void
Profile::LoadUnits()
{
  bool found = false;
  unsigned Lift = 0;
  unsigned Altitude = 0;
  unsigned Temperature = 0;
  unsigned Temp = 0;

  if (Get(szProfileLatLonUnits, Temp))
    Units::SetCoordinateFormat((CoordinateFormats)Temp);

  unsigned Speed = 1;
  found |= Get(szProfileSpeedUnitsValue, Speed);
  switch (Speed) {
  case 0:
    Units::SetUserSpeedUnit(unStatuteMilesPerHour);
    Units::SetUserWindSpeedUnit(unStatuteMilesPerHour);
    break;
  case 1:
    Units::SetUserSpeedUnit(unKnots);
    Units::SetUserWindSpeedUnit(unKnots);
    break;
  case 2:
  default:
    Units::SetUserSpeedUnit(unKiloMeterPerHour);
    Units::SetUserWindSpeedUnit(unKiloMeterPerHour);
    break;
  }

  unsigned TaskSpeed = 2;
  found |= Get(szProfileTaskSpeedUnitsValue, TaskSpeed);
  switch (TaskSpeed) {
  case 0:
    Units::SetUserTaskSpeedUnit(unStatuteMilesPerHour);
    break;
  case 1:
    Units::SetUserTaskSpeedUnit(unKnots);
    break;
  case 2:
  default:
    Units::SetUserTaskSpeedUnit(unKiloMeterPerHour);
    break;
  }

  unsigned Distance = 2;
  found |= Get(szProfileDistanceUnitsValue,Distance);
  switch (Distance) {
  case 0:
    Units::SetUserDistanceUnit(unStatuteMiles);
    break;
  case 1:
    Units::SetUserDistanceUnit(unNauticalMiles);
    break;
  case 2:
  default:
    Units::SetUserDistanceUnit(unKiloMeter);
    break;
  }

  found |= Get(szProfileAltitudeUnitsValue, Altitude);
  switch (Altitude) {
  case 0:
    Units::SetUserAltitudeUnit(unFeet);
    break;
  case 1:
  default:
    Units::SetUserAltitudeUnit(unMeter);
    break;
  }

  found |= Get(szProfileTemperatureUnitsValue, Temperature);
  switch (Temperature) {
  default:
  case 0:
    Units::SetUserTemperatureUnit(unGradCelcius);
    break;
  case 1:
    Units::SetUserTemperatureUnit(unGradFahrenheit);
    break;
  }

  found |= Get(szProfileLiftUnitsValue, Lift);
  switch (Lift) {
  case 0:
    Units::SetUserVerticalSpeedUnit(unKnots);
    break;
  case 2:
    Units::SetUserVerticalSpeedUnit(unFeetPerMinute);
    break;
  case 1:
  default:
    Units::SetUserVerticalSpeedUnit(unMeterPerSecond);
    break;
  }

  if (!found)
    Units::LoadFromOSLanguage();
}
