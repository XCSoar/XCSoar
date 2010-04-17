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

#if !defined(__UNITS_H)
#define __UNITS_H

#include <tchar.h>

#ifndef __GNUC__
#define DEG "°"
#else
#define DEG "Â°"
#endif

typedef enum {
  cfDDMMSS = 0,
  cfDDMMSSss,
  cfDDMMmmm,
  cfDDdddd
} CoordinateFormats_t;

typedef enum {
  unUndef,
  unKiloMeter,
  unNauticalMiles,
  unStatuteMiles,
  unKiloMeterPerHour,
  unKnots,
  unStatuteMilesPerHour,
  unMeterPerSecond,
  unFeetPerMinutes,
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
} Units_t;

typedef enum {
  ugNone,
  ugDistance,
  ugAltitude,
  ugHorizontalSpeed,
  ugVerticalSpeed,
  ugWindSpeed,
  ugTaskSpeed
} UnitGroup_t;

typedef struct{
  const TCHAR *Name;
  double ToUserFact;
  double ToUserOffset;
} UnitDescriptor_t;

/**
 * Class to manage unit conversions and display,
 * internal system units are (metric SI).
 * 
 */
class Units
{
private:
  static UnitDescriptor_t UnitDescriptors[unCount];

public:
  static Units_t DistanceUnit;  /**< Unit for distances */
  static Units_t AltitudeUnit; /**< Unit for altitudes, heights */
  static Units_t SpeedUnit; /**< Unit for aircraft speeds */
  static Units_t VerticalSpeedUnit; /**< Unit for vertical speeds, varios */
  static Units_t WindSpeedUnit; /**< Unit for wind speeds */
  static Units_t TaskSpeedUnit; /**< Unit for task speeds */
  static CoordinateFormats_t CoordinateFormat; /**< Unit for lat/lon */

  /**
   * Returns the name of the given Unit
   * @return The name of the given Unit (e.g. "km" or "ft")
   */
  static const TCHAR *GetUnitName(Units_t Unit);

  /**
   * Returns the user-specified coordinate format
   * @return The user-specified coordinate format
   */
  static CoordinateFormats_t GetCoordinateFormat(void);
  /**
   * Sets the user-specified coordinate format
   * @param NewUnit The new coordinate format
   * @return The old coordinate format
   */
  static CoordinateFormats_t SetCoordinateFormat(CoordinateFormats_t NewFormat);

  /**
   * Returns the user-specified unit for a horizontal distance
   * @return The user-specified unit for a horizontal distance
   */
  static Units_t GetUserDistanceUnit(void);
  /**
   * Sets the user-specified unit for a horizontal distance
   * @param NewUnit The new unit
   * @return The old unit
   */
  static Units_t SetUserDistanceUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for an altitude
   * @return The user-specified unit for an altitude
   */
  static Units_t GetUserAltitudeUnit(void);
  /**
   * Sets the user-specified unit for an altitude
   * @param NewUnit The new unit
   * @return The old unit
   */
  static Units_t SetUserAltitudeUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for a horizontal speed
   * @return The user-specified unit for a horizontal speed
   */
  static Units_t GetUserSpeedUnit(void);
  /**
   * Sets the user-specified unit for a horizontal speed
   * @param NewUnit The new unit
   * @return The old unit
   */
  static Units_t SetUserSpeedUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for a task speed
   * @return The user-specified unit for a task speed
   */
  static Units_t GetUserTaskSpeedUnit(void);
  /**
   * Sets the user-specified unit for a task speed
   * @param NewUnit The new unit
   * @return The old unit
   */
  static Units_t SetUserTaskSpeedUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for a vertical speed
   * @return The user-specified unit for a vertical speed
   */
  static Units_t GetUserVerticalSpeedUnit(void);
  /**
   * Sets the user-specified unit for a vertical speed
   * @param NewUnit The new unit
   * @return The old unit
   */
  static Units_t SetUserVerticalSpeedUnit(Units_t NewUnit);

  /**
   * Returns the user-specified unit for a wind speed
   * @return The user-specified unit for a wind speed
   */
  static Units_t GetUserWindSpeedUnit(void);
  /**
   * Sets the user-specified unit for a wind speed
   * @param NewUnit The new unit
   * @return The old unit
   */
  static Units_t SetUserWindSpeedUnit(Units_t NewUnit);

  static Units_t GetUserUnitByGroup(UnitGroup_t UnitGroup);

  /**
   * Converts a double-based Longitude to degrees, minute, seconds and a
   * bool-based east variable
   * @param Longitude The double-based Longitude to convert
   * @param dd Degrees (pointer)
   * @param mm Minutes (pointer)
   * @param ss Seconds (pointer)
   * @param east True if East, False if West (pointer)
   */
  static void LongitudeToDMS(double Longitude,
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
  static void LatitudeToDMS(double Latitude,
                            int *dd, int *mm, int *ss, bool *north);

  /**
   * Converts a double-based Longitude into a formatted string
   * @param Longitude The double-based Longitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   */
  static bool LongitudeToString(double Longitude, TCHAR *Buffer, size_t size);
  /**
   * Converts a double-based Latitude into a formatted string
   * @param Latitude The double-based Latitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   */
  static bool LatitudeToString(double Latitude, TCHAR *Buffer, size_t size);

  static void NotifyUnitChanged(void);

  static const TCHAR *GetSpeedName();
  static const TCHAR *GetVerticalSpeedName();
  static const TCHAR *GetDistanceName();
  static const TCHAR *GetAltitudeName();
  static const TCHAR *GetTaskSpeedName();

  /**
   * Converts a double-based Altitude into a formatted string
   * @param Altitude The double-based Altitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  static bool FormatUserAltitude(double Altitude, TCHAR *Buffer, size_t size,
                                 bool IncludeUnit = true);
  /**
   * Converts a double-based Altitude into a formatted string of the alternate
   * altitude format
   * @param Altitude The double-based Altitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  static bool FormatAlternateUserAltitude(double Altitude, TCHAR *Buffer,
                                          size_t size, bool IncludeUnit = true);
  /**
   * Converts a double-based Arrival Altitude into a formatted string
   * @param Altitude The double-based Arrival Altitude
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  static bool FormatUserArrival(double Altitude, TCHAR *Buffer, size_t size,
                                bool IncludeUnit = true);
  /**
   * Converts a double-based horizontal Distance into a formatted string
   * @param Distance The double-based Distance
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  static bool FormatUserDistance(double Distance, TCHAR *Buffer, size_t size,
                                 bool IncludeUnit = true);
  static bool FormatUserMapScale(Units_t *Unit, double Distance, TCHAR *Buffer,
                                 size_t size, bool IncludeUnit = true);
  /**
   * Converts a double-based Speed into a formatted string
   * @param Speed The double-based Speed
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  static bool FormatUserSpeed(double Altitude, TCHAR *Buffer, size_t size,
                              bool IncludeUnit = true);
  /**
   * Converts a double-based vertical Speed into a formatted string
   * @param Speed The double-based vertical Speed
   * @param Buffer Buffer string to write to (pointer)
   * @param size Size of the Buffer
   * @return True if Buffer long enough, False otherwise
   */
  static bool FormatUserVSpeed(double Altitude, TCHAR *Buffer, size_t size,
                               bool IncludeUnit = true);

  /**
   * Converts a Value from the given unit "From" to the given unit "To"
   * @param Value The value in system unit
   * @param From The source unit
   * @param To The destination unit
   * @return The value in user-specified unit
   */
  static double ConvertUnits(double Value, Units_t From, Units_t To);

  /**
   * Converts a Value from the system unit to the user-specified unit
   * @param Value The value in system unit
   * @param Unit The destination unit
   * @return The value in user-specified unit
   */
  static double ToUserUnit(double Value, Units_t Unit);
  /**
   * Converts a Value from the user-specified unit to the system unit
   * @param Value The value in user-specified unit
   * @param Unit The source unit
   * @return The value in system unit
   */
  static double ToSysUnit(double Value, Units_t Unit);

  static double ToUserAltitude(double Value) {
    return ToUserUnit(Value, AltitudeUnit);
  }

  static double ToSysAltitude(double Value) {
    return ToSysUnit(Value, AltitudeUnit);
  }

  static double ToUserDistance(double Value) {
    return ToUserUnit(Value, DistanceUnit);
  }

  static double ToSysDistance(double Value) {
    return ToSysUnit(Value, DistanceUnit);
  }

  static double ToUserSpeed(double Value) {
    return ToUserUnit(Value, SpeedUnit);
  }

  static double ToSysSpeed(double Value) {
    return ToSysUnit(Value, SpeedUnit);
  }

  static double ToUserVSpeed(double Value) {
    return ToUserUnit(Value, VerticalSpeedUnit);
  }

  static double ToSysVSpeed(double Value) {
    return ToSysUnit(Value, VerticalSpeedUnit);
  }

  static double ToUserTaskSpeed(double Value) {
    return ToUserUnit(Value, TaskSpeedUnit);
  }

  static double ToSysTaskSpeed(double Value) {
    return ToSysUnit(Value, TaskSpeedUnit);
  }

  static double ToUserWindSpeed(double Value) {
    return ToUserUnit(Value, WindSpeedUnit);
  }

  static double ToSysWindSpeed(double Value) {
    return ToSysUnit(Value, WindSpeedUnit);
  }

  static void TimeToText(TCHAR* text, int d);
};

#endif
