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

#include "Settings.hpp"

void
UnitSetting::SetDefaults()
{
  distance_unit = unKiloMeter;
  altitude_unit = unMeter;
  temperature_unit = unGradCelcius;
  speed_unit = unKiloMeterPerHour;
  vertical_speed_unit = unMeterPerSecond;
  wind_speed_unit = unKiloMeterPerHour;
  task_speed_unit = unKiloMeterPerHour;
  pressure_unit = unHectoPascal;
  coordinate_format = CF_DDMMSS;
}

Unit
UnitSetting::GetByGroup(UnitGroup group) const
{
  switch (group) {
  case ugNone:
    break;

  case ugDistance:
    return distance_unit;

  case ugAltitude:
    return altitude_unit;

  case ugTemperature:
    return temperature_unit;

  case ugHorizontalSpeed:
    return speed_unit;

  case ugVerticalSpeed:
    return vertical_speed_unit;

  case ugWindSpeed:
    return wind_speed_unit;

  case ugTaskSpeed:
    return task_speed_unit;

  case ugPressure:
    return pressure_unit;
  }

  return unUndef;
}

bool
UnitSetting::CompareUnitsOnly(const UnitSetting &right) const
{
  return (distance_unit == right.distance_unit &&
      altitude_unit == right.altitude_unit &&
      temperature_unit == right.temperature_unit &&
      speed_unit == right.speed_unit &&
      vertical_speed_unit == right.vertical_speed_unit &&
      wind_speed_unit == right.wind_speed_unit &&
      task_speed_unit == right.task_speed_unit &&
      pressure_unit == right.pressure_unit);
}
