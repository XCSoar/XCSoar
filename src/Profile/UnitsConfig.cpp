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

#include "UnitsConfig.hpp"
#include "ProfileKeys.hpp"
#include "Map.hpp"
#include "Units/Settings.hpp"
#include "Units/UnitsGlue.hpp"

static bool
ApplyUnit(Unit &value, Unit new_value)
{
  if (new_value == Unit::UNDEFINED)
    return false;

  value = new_value;
  return true;
}

/**
 * Convert XCSoar <= 6.2 profile value.
 */
gcc_const
static Unit
ImportSpeedUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return Unit::STATUTE_MILES_PER_HOUR;

  case 1:
    return Unit::KNOTS;

  case 2:
    return Unit::KILOMETER_PER_HOUR;

  default:
    return Unit::UNDEFINED;
  }
}

static bool
GetLegacySpeedUnit(const ProfileMap &map, const char *key, Unit &value)
{
  unsigned tmp;
  return map.Get(key, tmp) && ApplyUnit(value, ImportSpeedUnit(tmp));
}

gcc_const
static bool
ValidSpeedUnit(Unit unit)
{
  return unit == Unit::KILOMETER_PER_HOUR || unit == Unit::KNOTS ||
    unit == Unit::STATUTE_MILES_PER_HOUR || unit == Unit::METER_PER_SECOND ||
    unit == Unit::FEET_PER_MINUTE;
}

static bool
GetSpeedUnit(const ProfileMap &map, const char *key, const char *legacy_key,
             Unit &value_r)
{
  Unit tmp;
  if (!map.GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacySpeedUnit(map, legacy_key, value_r);

  if (!ValidSpeedUnit(tmp))
    return false;

  value_r = tmp;
  return true;
}

/**
 * Convert XCSoar <= 6.2 profile value.
 */
gcc_const
static Unit
ImportVerticalSpeedUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return Unit::KNOTS;

  case 1:
    return Unit::METER_PER_SECOND;

  case 2:
    return Unit::FEET_PER_MINUTE;

  default:
    return Unit::UNDEFINED;
  }
}

static bool
GetLegacyVerticalSpeedUnit(const ProfileMap &map, const char *key, Unit &value)
{
  unsigned tmp;
  return map.Get(key, tmp) && ApplyUnit(value, ImportVerticalSpeedUnit(tmp));
}

static bool
GetVerticalSpeedUnit(const ProfileMap &map, const char *key,
                     const char *legacy_key, Unit &value_r)
{
  Unit tmp;
  if (!map.GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacyVerticalSpeedUnit(map, legacy_key, value_r);

  if (!ValidSpeedUnit(tmp))
    return false;

  value_r = tmp;
  return true;
}

/**
 * Convert XCSoar <= 6.2 profile value.
 */
gcc_const
static Unit
ImportDistanceUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return Unit::STATUTE_MILES;

  case 1:
    return Unit::NAUTICAL_MILES;

  case 2:
    return Unit::KILOMETER;

  default:
    return Unit::UNDEFINED;
  }
}

static bool
GetLegacyDistanceUnit(const ProfileMap &map, const char *key, Unit &value)
{
  unsigned tmp;
  return map.Get(key, tmp) && ApplyUnit(value, ImportDistanceUnit(tmp));
}

gcc_const
static bool
ValidDistanceUnit(Unit unit)
{
  return unit == Unit::KILOMETER || unit == Unit::NAUTICAL_MILES ||
    unit == Unit::STATUTE_MILES || unit == Unit::METER ||
    unit == Unit::FEET || unit == Unit::FLIGHT_LEVEL;
}

static bool
GetDistanceUnit(const ProfileMap &map, const char *key, const char *legacy_key,
                Unit &value_r)
{
  Unit tmp;
  if (!map.GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacyDistanceUnit(map, legacy_key, value_r);

  if (!ValidDistanceUnit(tmp))
    return false;

  value_r = tmp;
  return true;
}

/**
 * Convert XCSoar <= 6.2 profile value.
 */
gcc_const
static Unit
ImportAltitudeUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return Unit::FEET;

  case 1:
    return Unit::METER;

  default:
    return Unit::UNDEFINED;
  }
}

static bool
GetLegacyAltitudeUnit(const ProfileMap &map, const char *key, Unit &value)
{
  unsigned tmp;
  return map.Get(key, tmp) && ApplyUnit(value, ImportAltitudeUnit(tmp));
}

static bool
GetAltitudeUnit(const ProfileMap &map, const char *key, const char *legacy_key,
                Unit &value_r)
{
  Unit tmp;
  if (!map.GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacyAltitudeUnit(map, legacy_key, value_r);

  if (!ValidDistanceUnit(tmp))
    return false;

  value_r = tmp;
  return true;
}

/**
 * Convert XCSoar <= 6.2 profile value.
 */
gcc_const
static Unit
ImportTemperatureUnit(unsigned tmp)
{
  switch (tmp) {
  case 0:
    return Unit::DEGREES_CELCIUS;

  case 1:
    return Unit::DEGREES_FAHRENHEIT;

  default:
    return Unit::UNDEFINED;
  }
}

static bool
GetLegacyTemperatureUnit(const ProfileMap &map, const char *key, Unit &value)
{
  unsigned tmp;
  return map.Get(key, tmp) &&
    ApplyUnit(value, ImportTemperatureUnit(tmp));
}

gcc_const
static bool
ValidTemperatureUnit(Unit unit)
{
  return unit == Unit::KELVIN || unit == Unit::DEGREES_CELCIUS ||
    unit == Unit::DEGREES_FAHRENHEIT;
}

static bool
GetTemperatureUnit(const ProfileMap &map, const char *key,
                   const char *legacy_key, Unit &value_r)
{
  Unit tmp;
  if (!map.GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacyTemperatureUnit(map, legacy_key, value_r);

  if (!ValidTemperatureUnit(tmp))
    return false;

  value_r = tmp;
  return true;
}

gcc_const
static bool
ValidPressureUnit(Unit unit)
{
  return unit == Unit::HECTOPASCAL || unit == Unit::MILLIBAR ||
    unit == Unit::TORR || unit == Unit::INCH_MERCURY;
}

static bool
GetPressureUnit(const ProfileMap &map, const char *key, Unit &value)
{
  Unit tmp;
  if (!map.GetEnum(key, tmp) || !ValidPressureUnit(tmp))
    return false;

  value = tmp;
  return true;
}

static constexpr bool
ValidWingLoadingUnit(Unit unit)
{
  return unit == Unit::KG_PER_M2 || unit == Unit::LB_PER_FT2;
}

static bool
GetWingLoadingUnit(const ProfileMap &map, const char *key, Unit &value)
{
  Unit tmp;
  if (!map.GetEnum(key, tmp) || !ValidWingLoadingUnit(tmp))
    return false;

  value = tmp;
  return true;
}

static constexpr bool
ValidMassUnit(Unit unit)
{
  return unit == Unit::KG || unit == Unit::LB;
}

static bool
GetMassUnit(const ProfileMap &map, const char *key, Unit &value)
{
  Unit tmp;
  if (!map.GetEnum(key, tmp) || !ValidMassUnit(tmp))
    return false;

  value = tmp;
  return true;
}

void
Profile::LoadUnits(const ProfileMap &map, UnitSetting &config)
{
  config = Units::LoadFromOSLanguage();

  GetSpeedUnit(map, ProfileKeys::SpeedUnitsValue, "Speed", config.speed_unit);
  config.wind_speed_unit = config.speed_unit;
  GetSpeedUnit(map, ProfileKeys::TaskSpeedUnitsValue, "TaskSpeed",
               config.task_speed_unit);
  GetDistanceUnit(map, ProfileKeys::DistanceUnitsValue, "Distance",
                  config.distance_unit);
  GetAltitudeUnit(map, ProfileKeys::AltitudeUnitsValue, "Altitude",
                  config.altitude_unit);
  GetTemperatureUnit(map, ProfileKeys::TemperatureUnitsValue, "Temperature",
                     config.temperature_unit);
  GetVerticalSpeedUnit(map, ProfileKeys::LiftUnitsValue, "Lift",
                       config.vertical_speed_unit);
  GetPressureUnit(map, ProfileKeys::PressureUnitsValue, config.pressure_unit);
  GetWingLoadingUnit(map, ProfileKeys::WingLoadingUnitValue,
                     config.wing_loading_unit);
  GetMassUnit(map, ProfileKeys::MassUnitValue, config.mass_unit);
}
