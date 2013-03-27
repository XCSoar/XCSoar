/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
GetLegacySpeedUnit(const char *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) && ApplyUnit(value, ImportSpeedUnit(tmp));
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
GetSpeedUnit(const char *key, const char *legacy_key, Unit &value_r)
{
  Unit tmp;
  if (!Profile::GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacySpeedUnit(legacy_key, value_r);

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
GetLegacyVerticalSpeedUnit(const char *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) && ApplyUnit(value, ImportVerticalSpeedUnit(tmp));
}

static bool
GetVerticalSpeedUnit(const char *key, const char *legacy_key, Unit &value_r)
{
  Unit tmp;
  if (!Profile::GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacyVerticalSpeedUnit(legacy_key, value_r);

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
GetLegacyDistanceUnit(const char *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) && ApplyUnit(value, ImportDistanceUnit(tmp));
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
GetDistanceUnit(const char *key, const char *legacy_key, Unit &value_r)
{
  Unit tmp;
  if (!Profile::GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacyDistanceUnit(legacy_key, value_r);

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
GetLegacyAltitudeUnit(const char *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) && ApplyUnit(value, ImportAltitudeUnit(tmp));
}

static bool
GetAltitudeUnit(const char *key, const char *legacy_key, Unit &value_r)
{
  Unit tmp;
  if (!Profile::GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacyAltitudeUnit(legacy_key, value_r);

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
GetLegacyTemperatureUnit(const char *key, Unit &value)
{
  unsigned tmp;
  return Profile::Get(key, tmp) &&
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
GetTemperatureUnit(const char *key, const char *legacy_key, Unit &value_r)
{
  Unit tmp;
  if (!Profile::GetEnum(key, tmp))
    /* migrate from XCSoar <= 6.2 profile */
    return GetLegacyTemperatureUnit(legacy_key, value_r);

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
GetPressureUnit(const char *key, Unit &value)
{
  Unit tmp;
  if (!Profile::GetEnum(key, tmp) || !ValidPressureUnit(tmp))
    return false;

  value = tmp;
  return true;
}

void
Profile::LoadUnits(UnitSetting &config)
{
  config = Units::LoadFromOSLanguage();

  GetSpeedUnit(ProfileKeys::SpeedUnitsValue, "Speed", config.speed_unit);
  config.wind_speed_unit = config.speed_unit;
  GetSpeedUnit(ProfileKeys::TaskSpeedUnitsValue, "TaskSpeed",
               config.task_speed_unit);
  GetDistanceUnit(ProfileKeys::DistanceUnitsValue, "Distance",
                  config.distance_unit);
  GetAltitudeUnit(ProfileKeys::AltitudeUnitsValue, "Altitude",
                  config.altitude_unit);
  GetTemperatureUnit(ProfileKeys::TemperatureUnitsValue, "Temperature",
                     config.temperature_unit);
  GetVerticalSpeedUnit(ProfileKeys::LiftUnitsValue, "Lift",
                       config.vertical_speed_unit);
  GetPressureUnit(ProfileKeys::PressureUnitsValue, config.pressure_unit);
}
