/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Math/fixed.hpp"
#include "Compiler.h"

#include <tchar.h>
class Angle;

#ifndef __GNUC__
#define DEG "°"
#else
#define DEG "Â°"
#endif

enum CoordinateFormats_t {
  cfDDMMSS = 0,
  cfDDMMSSss,
  cfDDMMmmm,
  cfDDdddd
};

enum Units_t {
  unUndef,
  unKiloMeter,
  unNauticalMiles,
  unStatuteMiles,
  unKiloMeterPerHour,
  unKnots,
  unStatuteMilesPerHour,
  unMeterPerSecond,
  unFeetPerMinute,
  unMeter,
  unFeet,
  unFlightLevel,
  unKelvin,
  unGradCelcius, // K = C° + 273,15
  unGradFahrenheit, // K = (°F + 459,67) / 1,8

  /**
   * The sentinel: the number of units in this enum.
   */
  unCount
};

enum UnitGroup_t
{
  ugNone,
  ugDistance,
  ugAltitude,
  ugTemperature,
  ugHorizontalSpeed,
  ugVerticalSpeed,
  ugWindSpeed,
  ugTaskSpeed
};

struct UnitDescriptor_t
{
  const TCHAR *Name;
  fixed ToUserFact;
  fixed ToUserOffset;
};

/**
 * Class to manage unit conversions and display,
 * internal system units are (metric SI).
 * 
 */
namespace Units
{
  extern const UnitDescriptor_t UnitDescriptors[];

  extern Units_t DistanceUnit;  /**< Unit for distances */
  extern Units_t AltitudeUnit; /**< Unit for altitudes, heights */
  extern Units_t TemperatureUnit; /**< Unit for temperature */
  extern Units_t SpeedUnit; /**< Unit for aircraft speeds */
  extern Units_t VerticalSpeedUnit; /**< Unit for vertical speeds, varios */
  extern Units_t WindSpeedUnit; /**< Unit for wind speeds */
  extern Units_t TaskSpeedUnit; /**< Unit for task speeds */
  extern CoordinateFormats_t CoordinateFormat; /**< Unit for lat/lon */

  /**
   * Returns the name of the given Unit
   * @return The name of the given Unit (e.g. "km" or "ft")
   */
  gcc_const
  const TCHAR *GetUnitName(Units_t Unit);

  /**
   * Returns the user-specified coordinate format
   * @return The user-specified coordinate format
   */
  gcc_const
  CoordinateFormats_t GetCoordinateFormat();
  /**
   * Sets the user-specified coordinate format
   * @param NewUnit The new coordinate format
   * @return The old coordinate format
   */
  CoordinateFormats_t SetCoordinateFormat(CoordinateFormats_t NewFormat);

  /**
   * Returns the user-specified unit for a horizontal distance
   * @return The user-specified unit for a horizontal distance
   */
  gcc_const
  Units_t GetUserDistanceUnit();
  /**
   * Sets the user-specified unit for a horizontal distance
   * @param NewUnit The new unit
   * @return The old unit
   */
  Units_t SetUserDistanceUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for an altitude
   * @return The user-specified unit for an altitude
   */
  gcc_const
  Units_t GetUserAltitudeUnit();

  /**
   * Sets the user-specified unit for an altitude
   * @param NewUnit The new unit
   * @return The old unit
   */
  Units_t SetUserAltitudeUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for a temperature
   * @return The user-specified unit for a temperature
   */
  gcc_const
  Units_t GetUserTemperatureUnit();

  /**
   * Sets the user-specified unit for a temperature
   * @param NewUnit The new unit
   * @return The old unit
   */
  Units_t SetUserTemperatureUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for a horizontal speed
   * @return The user-specified unit for a horizontal speed
   */
  gcc_const
  Units_t GetUserSpeedUnit();

  /**
   * Sets the user-specified unit for a horizontal speed
   * @param NewUnit The new unit
   * @return The old unit
   */
  Units_t SetUserSpeedUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for a task speed
   * @return The user-specified unit for a task speed
   */
  gcc_const
  Units_t GetUserTaskSpeedUnit();

  /**
   * Sets the user-specified unit for a task speed
   * @param NewUnit The new unit
   * @return The old unit
   */
  Units_t SetUserTaskSpeedUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for a vertical speed
   * @return The user-specified unit for a vertical speed
   */
  gcc_const
  Units_t GetUserVerticalSpeedUnit();

  /**
   * Sets the user-specified unit for a vertical speed
   * @param NewUnit The new unit
   * @return The old unit
   */
  Units_t SetUserVerticalSpeedUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for a wind speed
   * @return The user-specified unit for a wind speed
   */
  gcc_const
  Units_t GetUserWindSpeedUnit();

  /**
   * Sets the user-specified unit for a wind speed
   * @param NewUnit The new unit
   * @return The old unit
   */
  Units_t SetUserWindSpeedUnit(Units_t NewUnit);

  gcc_const
  Units_t GetUserUnitByGroup(UnitGroup_t UnitGroup);

  /**
   * Converts a double-based Longitude to degrees, minute, seconds and a
   * bool-based east variable
   * @param Longitude The double-based Longitude to convert
   * @param dd Degrees (pointer)
   * @param mm Minutes (pointer)
   * @param ss Seconds (pointer)
   * @param east True if East, False if West (pointer)
   */
  void LongitudeToDMS(Angle Longitude,
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
  void LatitudeToDMS(Angle Latitude,
                     int *dd, int *mm, int *ss, bool *north);

  /**
   * Converts a double-based Longitude into a formatted string
   * @param Longitude The double-based Longitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   */
  bool LongitudeToString(Angle Longitude, TCHAR *Buffer, size_t size);

  /**
   * Converts a double-based Latitude into a formatted string
   * @param Latitude The double-based Latitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   */
  bool LatitudeToString(Angle Latitude, TCHAR *Buffer, size_t size);

  gcc_const
  const TCHAR *GetSpeedName();

  gcc_const
  const TCHAR *GetVerticalSpeedName();

  gcc_const
  const TCHAR *GetDistanceName();

  gcc_const
  const TCHAR *GetAltitudeName();
  gcc_const

  gcc_const
  const TCHAR *GetTemperatureName();

  gcc_const
  const TCHAR *GetTaskSpeedName();

  /**
   * Converts a double-based Altitude into a formatted string
   * @param Altitude The double-based Altitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserAltitude(fixed Altitude, TCHAR *Buffer, size_t size,
                          bool IncludeUnit = true);

  /**
   * Converts a double-based Altitude into a formatted string of the alternate
   * altitude format
   * @param Altitude The double-based Altitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatAlternateUserAltitude(fixed Altitude, TCHAR *Buffer,
                                   size_t size, bool IncludeUnit = true);

  /**
   * Converts a double-based Arrival Altitude into a formatted string
   * @param Altitude The double-based Arrival Altitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserArrival(fixed Altitude, TCHAR *Buffer, size_t size,
                         bool IncludeUnit = true);

  /**
   * Converts a double-based horizontal Distance into a formatted string
   * @param Distance The double-based Distance
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserDistance(fixed Distance, TCHAR *Buffer, size_t size,
                          bool IncludeUnit = true);

  bool FormatUserMapScale(Units_t *Unit, fixed Distance, TCHAR *Buffer,
                          size_t size, bool IncludeUnit = true);

  /**
   * Converts a double-based Speed into a formatted string
   * @param Speed The double-based Speed
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserSpeed(fixed Altitude, TCHAR *Buffer, size_t size,
                       bool IncludeUnit = true);

  /**
   * Converts a double-based vertical Speed into a formatted string
   * @param Speed The double-based vertical Speed
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  bool FormatUserVSpeed(fixed Altitude, TCHAR *Buffer, size_t size,
                        bool IncludeUnit = true);

  /**
   * Converts a Value from the given unit "From" to the given unit "To"
   * @param Value The value in system unit
   * @param From The source unit
   * @param To The destination unit
   * @return The value in user-specified unit
   */
  gcc_const
  fixed ConvertUnits(fixed Value, Units_t From, Units_t To);

  /**
   * Converts a Value from the system unit to the user-specified unit
   * @param Value The value in system unit
   * @param Unit The destination unit
   * @return The value in user-specified unit
   */
  gcc_const
  fixed ToUserUnit(fixed Value, Units_t Unit);

  /**
   * Converts a Value from the user-specified unit to the system unit
   * @param Value The value in user-specified unit
   * @param Unit The source unit
   * @return The value in system unit
   */
  gcc_const
  fixed ToSysUnit(fixed Value, Units_t Unit);

  static inline fixed
  ToUserAltitude(fixed Value)
  {
    return ToUserUnit(Value, AltitudeUnit);
  }

  static inline fixed
  ToSysAltitude(fixed Value)
  {
    return ToSysUnit(Value, AltitudeUnit);
  }

  static inline fixed
  ToUserTemperature(fixed Value)
  {
    return ToUserUnit(Value, TemperatureUnit);
  }

  static inline fixed
  ToSysTemperature(fixed Value)
  {
    return ToSysUnit(Value, TemperatureUnit);
  }

  static inline fixed
  ToUserDistance(fixed Value)
  {
    return ToUserUnit(Value, DistanceUnit);
  }

  static inline fixed
  ToSysDistance(fixed Value)
  {
    return ToSysUnit(Value, DistanceUnit);
  }

  static inline fixed
  ToUserSpeed(fixed Value)
  {
    return ToUserUnit(Value, SpeedUnit);
  }

  static inline fixed
  ToSysSpeed(fixed Value)
  {
    return ToSysUnit(Value, SpeedUnit);
  }

  static inline fixed
  ToUserVSpeed(fixed Value)
  {
    return ToUserUnit(Value, VerticalSpeedUnit);
  }

  static inline fixed
  ToSysVSpeed(fixed Value)
  {
    return ToSysUnit(Value, VerticalSpeedUnit);
  }

  static inline fixed
  ToUserTaskSpeed(fixed Value)
  {
    return ToUserUnit(Value, TaskSpeedUnit);
  }

  static inline fixed
  ToSysTaskSpeed(fixed Value)
  {
    return ToSysUnit(Value, TaskSpeedUnit);
  }

  static inline fixed
  ToUserWindSpeed(fixed Value)
  {
    return ToUserUnit(Value, WindSpeedUnit);
  }

  static inline fixed
  ToSysWindSpeed(fixed Value)
  {
    return ToSysUnit(Value, WindSpeedUnit);
  }

  void TimeToText(TCHAR* text, int d);
};

#endif
