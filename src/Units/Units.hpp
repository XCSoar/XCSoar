// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Settings.hpp"
#include "System.hpp"

#include <tchar.h>

class AtmosphericPressure;

#define DEG "°"

/**
 * Namespace to manage unit conversions.
 * internal system units are (metric SI).
 */
namespace Units {

extern UnitSetting current;

void
SetConfig(const UnitSetting &new_config);

/**
 * Returns the user-specified unit for a wing loading
 * @return The user-specified unit for a wing loading
 */
[[gnu::pure]]
Unit
GetUserWingLoadingUnit();

/**
 * Returns the user-specified unit for mass
 * @return The user-specified unit for mass
 */
[[gnu::pure]]
Unit
GetUserMassUnit();

/**
 * Returns the user-specified unit for a horizontal distance
 * @return The user-specified unit for a horizontal distance
 */
[[gnu::pure]]
Unit
GetUserDistanceUnit();

/**
 * Returns the user-specified unit for an altitude
 * @return The user-specified unit for an altitude
 */
[[gnu::pure]]
Unit
GetUserAltitudeUnit();

/**
 * Returns the user-specified unit for a temperature
 * @return The user-specified unit for a temperature
 */
[[gnu::pure]]
Unit
GetUserTemperatureUnit();

/**
 * Returns the user-specified unit for a horizontal speed
 * @return The user-specified unit for a horizontal speed
 */
[[gnu::pure]]
Unit
GetUserSpeedUnit();

/**
 * Returns the user-specified unit for a task speed
 * @return The user-specified unit for a task speed
 */
[[gnu::pure]]
Unit
GetUserTaskSpeedUnit();

/**
 * Returns the user-specified unit for a vertical speed
 * @return The user-specified unit for a vertical speed
 */
[[gnu::pure]]
Unit
GetUserVerticalSpeedUnit();

/**
 * Returns the user-specified unit for a wind speed
 * @return The user-specified unit for a wind speed
 */
[[gnu::pure]]
Unit
GetUserWindSpeedUnit();

/**
 * Returns the user-specified unit for a pressure
 * @return The user-specified unit for a pressure
 */
[[gnu::pure]]
Unit
GetUserPressureUnit();

[[gnu::pure]]
Unit
GetUserUnitByGroup(UnitGroup group);

[[gnu::pure]]
const char *
GetSpeedName();

[[gnu::pure]]
const char *
GetVerticalSpeedName();

[[gnu::pure]]
const char *
GetWindSpeedName();

[[gnu::pure]]
const char *
GetDistanceName();

[[gnu::pure]]
const char *
GetAltitudeName();

[[gnu::pure]]
const char *
GetTemperatureName();

[[gnu::pure]]
const char *
GetTaskSpeedName();

[[gnu::pure]]
const char *
GetPressureName();

static inline double
ToUserAltitude(double value)
{
  return ToUserUnit(value, current.altitude_unit);
}

static inline double
ToSysAltitude(double value)
{
  return ToSysUnit(value, current.altitude_unit);
}

static inline double
ToUserDistance(double value)
{
  return ToUserUnit(value, current.distance_unit);
}

static inline double
ToSysDistance(double value)
{
  return ToSysUnit(value, current.distance_unit);
}

static inline double
ToUserSpeed(double value)
{
  return ToUserUnit(value, current.speed_unit);
}

static inline double
ToSysSpeed(double value)
{
  return ToSysUnit(value, current.speed_unit);
}

static inline double
ToUserVSpeed(double value)
{
  return ToUserUnit(value, current.vertical_speed_unit);
}

static inline double
ToSysVSpeed(double value)
{
  return ToSysUnit(value, current.vertical_speed_unit);
}

static inline double
ToUserTaskSpeed(double value)
{
  return ToUserUnit(value, current.task_speed_unit);
}

static inline double
ToSysTaskSpeed(double value)
{
  return ToSysUnit(value, current.task_speed_unit);
}

static inline double
ToUserWindSpeed(double value)
{
  return ToUserUnit(value, current.wind_speed_unit);
}

static inline double
ToSysWindSpeed(double value)
{
  return ToSysUnit(value, current.wind_speed_unit);
}

static inline double
ToUserPressure(double Value)
{
  return ToUserUnit(Value, current.pressure_unit);
}
  
[[gnu::const]]
double
ToUserPressure(AtmosphericPressure value);

static inline double
ToSysPressure(double Value)
{
  return ToSysUnit(Value, current.pressure_unit);
}

/**
 * Convert a pressure value from the user unit to an
 * #AtmosphericPressure object.
 */
[[gnu::const]]
AtmosphericPressure
FromUserPressure(double value);
  
static inline double
ToUserMass(double Value)
{
  return ToUserUnit(Value, current.mass_unit);
}
  
static inline double
ToSysMass(double Value)
{
  return ToSysUnit(Value, current.mass_unit);
}

static inline double
ToUserRotation(double Value) noexcept
{
  return ToUserUnit(Value, current.rotation_unit);
}

static inline double
ToSysRotation(double Value) noexcept
{
  return ToSysUnit(Value, current.rotation_unit);
}

} // namespace Units
