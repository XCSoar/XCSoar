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
  rotation_unit = Unit::RPM;
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

  case UnitGroup::ROTATION:
    return rotation_unit;
  }

  return Unit::UNDEFINED;
}
