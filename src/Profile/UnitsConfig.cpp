// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UnitsConfig.hpp"
#include "Keys.hpp"
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
static constexpr Unit
ImportSpeedUnit(unsigned tmp) noexcept
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

static constexpr bool
ValidSpeedUnit(Unit unit) noexcept
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
static constexpr Unit
ImportVerticalSpeedUnit(unsigned tmp) noexcept
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
static constexpr Unit
ImportDistanceUnit(unsigned tmp) noexcept
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

static constexpr bool
ValidDistanceUnit(Unit unit) noexcept
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
static constexpr Unit
ImportAltitudeUnit(unsigned tmp) noexcept
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
static constexpr Unit
ImportTemperatureUnit(unsigned tmp) noexcept
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

static constexpr bool
ValidTemperatureUnit(Unit unit) noexcept
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

static constexpr bool
ValidPressureUnit(Unit unit) noexcept
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

static constexpr bool
ValidRotationUnit(Unit unit) noexcept
{
  return unit == Unit::HZ || unit == Unit::RPM;
}

static bool
GetRotationUnit(const ProfileMap &map, const char *key, Unit &value)
{
  Unit tmp;
  if (!map.GetEnum(key, tmp) || !ValidRotationUnit(tmp))
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
  GetRotationUnit(map, ProfileKeys::RotationUnitValue, config.rotation_unit);
}
