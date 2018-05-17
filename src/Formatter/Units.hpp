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

#ifndef XCSOAR_UNITS_FORMATTER_HPP
#define XCSOAR_UNITS_FORMATTER_HPP

#include "Units/Unit.hpp"

#include <tchar.h>

class AtmosphericPressure;

/**
 * Converts an altitude into a formatted string
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the altitude
 * @param unit the altitude unit (e.g. meters, feet, ...)
 * @param include_unit include the unit into the string?
 */
void
FormatAltitude(TCHAR *buffer, double value, Unit unit,
               bool include_unit = true);

/**
 * Converts a mass into a formatted string
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the mass
 * @param unit the mass unit (e.g. kg, ft, ...)
 * @param include_unit include the unit into the string?
 */
void
FormatMass(TCHAR *buffer, double value, Unit unit,
           bool include_unit = true);

/**
 * Converts a wing loading into a formatted string
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the wing loading
 * @param unit the wing loading unit (e.g. kg/m2, ...)
 * @param include_unit include the unit into the string?
 */
void
FormatWingLoading(TCHAR *buffer, double value, Unit unit,
                  bool include_unit = true);

/**
 * Converts an signed/relative altitude into a formatted string
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the relative altitude
 * @param unit the altitude unit (e.g. meters, feet, ...)
 * @param include_unit include the unit into the string?
 */
void
FormatRelativeAltitude(TCHAR *buffer, double value, Unit unit,
                       bool include_unit = true);

/**
 * Converts a distance into a formatted string
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the distance
 * @param unit the distance unit (e.g. m, ft, km, nm, sm)
 * @param include_unit include the unit into the string?
 * @param precision the number of decimal places
 */
void
FormatDistance(TCHAR *buffer, double value, const Unit unit,
               bool include_unit = true, int precision = 0);

/**
 * Converts a distance into a formatted string using the smaller version
 * of the distance unit (km -> m, nm -> ft, sm -> ft)
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the distance
 * @param unit the distance unit (e.g. m, ft, km, nm, sm)
 * @param include_unit include the unit into the string?
 * @param precision the number of decimal places
 * @return the unit used for output formatting
 */
Unit
FormatSmallDistance(TCHAR *buffer, double value, Unit unit,
                    bool include_unit = true, int precision = 0);

/**
 * Converts a distance into a formatted string. Changes the unit if
 * the numbers get small.
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the distance
 * @param unit the distance unit (e.g. m, ft, km, nm, sm)
 * @param include_unit include the unit into the string?
 * @return the unit used for output formatting
 */
Unit
FormatDistanceSmart(TCHAR *buffer, double value, Unit unit,
                    bool include_unit = true,
                    double small_unit_threshold = 0,
                    double precision_threshold = 100);

/**
 * Converts a speed into a formatted string
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the speed
 * @param unit the speed unit (e.g. km/h, knots, mph, ...)
 * @param include_unit include the unit into the string?
 * @param precision if true shows one decimal place if the speed is low
 */
void
FormatSpeed(TCHAR *buffer, double value, const Unit unit,
            bool include_unit = true, bool precision = false);

/**
 * Returns the proper format to display the vertical speed
 * @param unit the pressure unit
 * @param include_unit include the unit into the string?
 * @param include_sign include the sign into the string?
 * @return the format
 */
const TCHAR* GetVerticalSpeedFormat(Unit unit, bool include_unit = false,
                                    bool include_sign = true);

/**
 * Give the basic step size for pressure editing
 * @param unit the pressure unit
 * @return the step size
 */
double
GetVerticalSpeedStep(Unit unit);

/**
 * Converts a vertical speed into a formatted string
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the vertical speed
 * @param unit the vertical speed unit (e.g. m/s, feet/min, ...)
 * @param include_unit include the unit into the string?
 * @param include_sign include the sign into the string?
 */
void
FormatVerticalSpeed(TCHAR *buffer, double value, Unit unit,
                    bool include_unit = true, bool include_sign = true);

/**
 * Converts a temperature into a formatted string
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the temperature
 * @param unit the temperature unit
 * @param include_unit include the unit into the string?
 */
void
FormatTemperature(TCHAR *buffer, double value, Unit unit,
                  bool include_unit = true);

/**
 * Converts a pressure into a formatted string
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the pressure
 * @param unit the pressure unit (e.g. meters, feet, ...)
 * @param include_unit include the unit into the string?
 */
void FormatPressure(TCHAR *buffer, AtmosphericPressure value, Unit unit,
                    bool include_unit = true);

/**
 * Returns the proper format to display the pressure
 * @param unit the pressure unit
 * @return the format
 */
const TCHAR* GetPressureFormat(Unit unit, bool include_unit = false);

/**
 * Give the basic step size for pressure editing
 * @param unit the pressure unit
 * @return the step size
 */
double
GetPressureStep(Unit unit);

#endif
