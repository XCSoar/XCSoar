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
  UnitSetting &config = Units::current;

  bool found = false;
  unsigned Lift = 0;
  unsigned Altitude = 0;
  unsigned Temperature = 0;

  GetEnum(szProfileLatLonUnits, config.coordinate_format);

  unsigned Speed = 1;
  found |= Get(szProfileSpeedUnitsValue, Speed);
  switch (Speed) {
  case 0:
    config.speed_unit = config.wind_speed_unit = unStatuteMilesPerHour;
    break;
  case 1:
    config.speed_unit = config.wind_speed_unit = unKnots;
    break;
  case 2:
  default:
    config.speed_unit = config.wind_speed_unit = unKiloMeterPerHour;
    break;
  }

  unsigned TaskSpeed = 2;
  found |= Get(szProfileTaskSpeedUnitsValue, TaskSpeed);
  switch (TaskSpeed) {
  case 0:
    config.task_speed_unit = unStatuteMilesPerHour;
    break;
  case 1:
    config.task_speed_unit = unKnots;
    break;
  case 2:
  default:
    config.task_speed_unit = unKiloMeterPerHour;
    break;
  }

  unsigned Distance = 2;
  found |= Get(szProfileDistanceUnitsValue,Distance);
  switch (Distance) {
  case 0:
    config.distance_unit = unStatuteMiles;
    break;
  case 1:
    config.distance_unit = unNauticalMiles;
    break;
  case 2:
  default:
    config.distance_unit = unKiloMeter;
    break;
  }

  found |= Get(szProfileAltitudeUnitsValue, Altitude);
  switch (Altitude) {
  case 0:
    config.altitude_unit = unFeet;
    break;
  case 1:
  default:
    config.altitude_unit = unMeter;
    break;
  }

  found |= Get(szProfileTemperatureUnitsValue, Temperature);
  switch (Temperature) {
  default:
  case 0:
    config.temperature_unit = unGradCelcius;
    break;
  case 1:
    config.temperature_unit = unGradFahrenheit;
    break;
  }

  found |= Get(szProfileLiftUnitsValue, Lift);
  switch (Lift) {
  case 0:
    config.vertical_speed_unit = unKnots;
    break;
  case 2:
    config.vertical_speed_unit = unFeetPerMinute;
    break;
  case 1:
  default:
    config.vertical_speed_unit = unMeterPerSecond;
    break;
  }

  found |= GetEnum(szProfilePressureUnitsValue, Units::current.pressure_unit);

  if (!found)
    config = Units::LoadFromOSLanguage();
}
