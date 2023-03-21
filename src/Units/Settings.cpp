// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"

void
UnitSetting::SetDefaults() noexcept
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
UnitSetting::GetByGroup(UnitGroup group) const noexcept
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
UnitSetting::operator==(const UnitSetting &right) const noexcept
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
