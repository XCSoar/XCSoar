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

#ifndef XCSOAR_UNITS_HPP
#define XCSOAR_UNITS_HPP

#include "Units/Settings.hpp"
#include "Math/fixed.hpp"
#include "Compiler.h"

#include <tchar.h>
class Angle;

#define DEG "°"

struct UnitDescriptor
{
  const TCHAR *name;
  fixed factor_to_user;
  fixed offset_to_user;
};

/**
 * Namespace to manage unit conversions.
 * internal system units are (metric SI).
 */
namespace Units
{
  extern const UnitDescriptor unit_descriptors[];

  extern UnitSetting current;

  /**
   * Returns the name of the given Unit
   * @return The name of the given Unit (e.g. "km" or "ft")
   */
  gcc_const
  const TCHAR *GetUnitName(Unit unit);

  /**
   * Returns the user-specified coordinate format
   * @return The user-specified coordinate format
   */
  gcc_pure
  CoordinateFormats GetCoordinateFormat();

  /**
   * Returns the user-specified unit for a horizontal distance
   * @return The user-specified unit for a horizontal distance
   */
  gcc_pure
  Unit GetUserDistanceUnit();

  /**
   * Returns the user-specified unit for an altitude
   * @return The user-specified unit for an altitude
   */
  gcc_pure
  Unit GetUserAltitudeUnit();

  /**
   * Returns the user-specified unit for a temperature
   * @return The user-specified unit for a temperature
   */
  gcc_pure
  Unit GetUserTemperatureUnit();

  /**
   * Returns the user-specified unit for a horizontal speed
   * @return The user-specified unit for a horizontal speed
   */
  gcc_pure
  Unit GetUserSpeedUnit();

  /**
   * Returns the user-specified unit for a task speed
   * @return The user-specified unit for a task speed
   */
  gcc_pure
  Unit GetUserTaskSpeedUnit();

  /**
   * Returns the user-specified unit for a vertical speed
   * @return The user-specified unit for a vertical speed
   */
  gcc_pure
  Unit GetUserVerticalSpeedUnit();

  /**
   * Returns the user-specified unit for a wind speed
   * @return The user-specified unit for a wind speed
   */
  gcc_pure
  Unit GetUserWindSpeedUnit();

  /**
   * Returns the user-specified unit for a pressure
   * @return The user-specified unit for a pressure
   */
  gcc_pure
  Unit GetUserPressureUnit();

  gcc_pure
  Unit GetUserUnitByGroup(UnitGroup group);

  /**
   * Converts a double-based Longitude to degrees, minute, seconds and a
   * bool-based east variable
   * @param Longitude The double-based Longitude to convert
   * @param dd Degrees (pointer)
   * @param mm Minutes (pointer)
   * @param ss Seconds (pointer)
   * @param east True if East, False if West (pointer)
   */
  void LongitudeToDMS(Angle longitude,
                      int *dd, int *mm, int *ss, bool *east);

  /**
   * Converts a double-based Latitude to degrees, minute, seconds and a
   * bool-based north variable
   * @param Latitude The double-based Latitude to convert
   * @param dd Degrees (pointer)
   * @param mm Minutes (pointer)
   * @param ss Seconds (pointer)
   * @param north True if North, False if South (pointer)
   */
  void LatitudeToDMS(Angle latitude,
                     int *dd, int *mm, int *ss, bool *north);

  gcc_pure
  const TCHAR *GetSpeedName();

  gcc_pure
  const TCHAR *GetVerticalSpeedName();

  gcc_pure
  const TCHAR *GetDistanceName();

  gcc_pure
  const TCHAR *GetAltitudeName();

  gcc_pure
  const TCHAR *GetTemperatureName();

  gcc_pure
  const TCHAR *GetTaskSpeedName();

  gcc_pure
  const TCHAR *GetPressureName();

  /**
   * Converts a value from the system unit to the user-specified unit
   * @param value The value in system unit
   * @param Unit The destination unit
   * @return The value in user-specified unit
   */
  gcc_const
  fixed ToUserUnit(fixed value, Unit unit);

  /**
   * Converts a value from the user-specified unit to the system unit
   * @param value The value in user-specified unit
   * @param Unit The source unit
   * @return The value in system unit
   */
  gcc_const
  fixed ToSysUnit(fixed value, Unit unit);

  static inline fixed
  ToUserAltitude(fixed value)
  {
    return ToUserUnit(value, current.altitude_unit);
  }

  static inline fixed
  ToSysAltitude(fixed value)
  {
    return ToSysUnit(value, current.altitude_unit);
  }

  static inline fixed
  ToUserTemperature(fixed value)
  {
    return ToUserUnit(value, current.temperature_unit);
  }

  static inline fixed
  ToSysTemperature(fixed value)
  {
    return ToSysUnit(value, current.temperature_unit);
  }

  static inline fixed
  ToUserDistance(fixed value)
  {
    return ToUserUnit(value, current.distance_unit);
  }

  static inline fixed
  ToSysDistance(fixed value)
  {
    return ToSysUnit(value, current.distance_unit);
  }

  static inline fixed
  ToUserSpeed(fixed value)
  {
    return ToUserUnit(value, current.speed_unit);
  }

  static inline fixed
  ToSysSpeed(fixed value)
  {
    return ToSysUnit(value, current.speed_unit);
  }

  static inline fixed
  ToUserVSpeed(fixed value)
  {
    return ToUserUnit(value, current.vertical_speed_unit);
  }

  static inline fixed
  ToSysVSpeed(fixed value)
  {
    return ToSysUnit(value, current.vertical_speed_unit);
  }

  static inline fixed
  ToUserTaskSpeed(fixed value)
  {
    return ToUserUnit(value, current.task_speed_unit);
  }

  static inline fixed
  ToSysTaskSpeed(fixed value)
  {
    return ToSysUnit(value, current.task_speed_unit);
  }

  static inline fixed
  ToUserWindSpeed(fixed value)
  {
    return ToUserUnit(value, current.wind_speed_unit);
  }

  static inline fixed
  ToSysWindSpeed(fixed value)
  {
    return ToSysUnit(value, current.wind_speed_unit);
  }

  static inline fixed
  ToUserPressure(fixed Value)
  {
    return ToUserUnit(Value, current.pressure_unit);
  }

  static inline fixed
  ToSysPressure(fixed Value)
  {
    return ToSysUnit(Value, current.pressure_unit);
  }
};

#endif
