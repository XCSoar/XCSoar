/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
  distance_unit = Unit::KILOMETER;
  altitude_unit = Unit::METER;
  temperature_unit = Unit::DEGREES_CELCIUS;
  speed_unit = Unit::KILOMETER_PER_HOUR;
  vertical_speed_unit = Unit::METER_PER_SECOND;
  wind_speed_unit = Unit::KILOMETER_PER_HOUR;
  task_speed_unit = Unit::KILOMETER_PER_HOUR;
  pressure_unit = Unit::HECTOPASCAL;
  wing_loading_unit = Unit::KG_PER_M2;
  mass_unit = Unit::KG;
}

Unit
UnitSetting::GetByGroup(UnitGroup group) const
{
  switch (group) {
  case UnitGroup::NONE:
    break;

  case UnitGroup::DISTANCE:
    return distance_unit;

  case UnitGroup::ALTITUDE:
    return altitude_unit;

  case UnitGroup::TEMPERATURE:
    return temperature_unit;

  case UnitGroup::HORIZONTAL_SPEED:
    return speed_unit;

  case UnitGroup::VERTICAL_SPEED:
    return vertical_speed_unit;

  case UnitGroup::WIND_SPEED:
    return wind_speed_unit;

  case UnitGroup::TASK_SPEED:
    return task_speed_unit;

  case UnitGroup::PRESSURE:
    return pressure_unit;

  case UnitGroup::WING_LOADING:
    return wing_loading_unit;

  case UnitGroup::MASS:
    return mass_unit;
  }

  return Unit::UNDEFINED;
}

bool
UnitSetting::operator==(const UnitSetting &right) const
{
  return (distance_unit == right.distance_unit &&
      altitude_unit == right.altitude_unit &&
      temperature_unit == right.temperature_unit &&
      speed_unit == right.speed_unit &&
      vertical_speed_unit == right.vertical_speed_unit &&
      wind_speed_unit == right.wind_speed_unit &&
      task_speed_unit == right.task_speed_unit &&
      pressure_unit == right.pressure_unit &&
      wing_loading_unit == right.wing_loading_unit &&
      mass_unit == right.mass_unit);
}
