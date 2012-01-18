/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Math/fixed.hpp"

#include <tchar.h>

class AtmosphericPressure;

/**
 * Namespace to manage unit display.
 */
namespace Units
{
  /**
   * Converts an altitude into a formatted string
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   * @param value the altitude
   * @param unit the altitude unit (e.g. meters, feet, ...)
   * @param include_unit include the unit into the string?
   */
  void FormatAltitude(TCHAR *buffer, size_t size, fixed value, Unit unit,
                      bool include_unit = true);

  /**
   * Converts a double-based Altitude into a formatted string
   * @param Altitude The double-based Altitude
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   */
  void FormatUserAltitude(fixed value, TCHAR *buffer, size_t size,
                          bool include_unit = true);

  /**
   * Converts a double-based Altitude into a formatted string of the alternate
   * altitude format
   * @param Altitude The double-based Altitude
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   */
  void FormatAlternateUserAltitude(fixed value, TCHAR *buffer, size_t size,
                                   bool include_unit = true);

  /**
   * Converts an signed/relative altitude into a formatted string
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   * @param value the relative altitude
   * @param unit the altitude unit (e.g. meters, feet, ...)
   * @param include_unit include the unit into the string?
   */
  void FormatRelativeAltitude(TCHAR *buffer, size_t size, fixed value,
                              Unit unit, bool include_unit = true);

  /**
   * Converts a double-based Arrival Altitude into a formatted string
   * @param Altitude The double-based Arrival Altitude
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   */
  void FormatUserArrival(fixed value, TCHAR *buffer, size_t size,
                         bool include_unit = true);

  /**
   * Converts a double-based horizontal Distance into a formatted string
   * @param Distance The double-based Distance
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   */
  void FormatUserDistance(fixed value, TCHAR *buffer, size_t size,
                          bool include_unit = true);

  void FormatUserMapScale(fixed value, TCHAR *buffer, size_t size,
                          bool include_unit = true);

  /**
   * Converts a speed into a formatted string
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   * @param value the speed
   * @param unit the speed unit (e.g. km/h, knots, mph, ...)
   * @param include_unit include the unit into the string?
   * @param precision if true shows one decimal place if the speed is low
   */
  void FormatSpeed(TCHAR *buffer, size_t size, fixed value, const Unit unit,
                   bool include_unit = true, bool precision = false);

  /**
   * Converts a double-based Speed into a formatted string
   * @param Speed The double-based Speed
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   * @return True if buffer long enough, False otherwise
   */
  void FormatUserSpeed(fixed value, TCHAR *buffer, size_t size,
                       bool include_unit = true, bool Precision = true);

  /**
   * Converts a double-based Speed into a formatted string
   * @param Speed The double-based Speed
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   * @return True if buffer long enough, False otherwise
   */
  void FormatUserWindSpeed(fixed value, TCHAR *buffer, size_t size,
                           bool include_unit = true, bool Precision = true);

  /**
   * Convert a speed [m/s] to the user's task speed and format it into
   * a string.
   *
   * @param value the speed value [m/s]
   */
  void FormatUserTaskSpeed(fixed value, TCHAR *buffer, size_t max_size,
                           bool include_unit=true, bool precision=true);

  /**
   * Converts a double-based vertical Speed into a formatted string
   * @param Speed The double-based vertical Speed
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   * @return True if buffer long enough, False otherwise
   */
  void FormatUserVSpeed(fixed value, TCHAR *buffer, size_t size,
                        bool include_unit = true);

  /**
   * Converts a temperature into a formatted string
   * @param temperature The double-based vertical Speed
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   */
  void FormatUserTemperature(fixed value, TCHAR *buffer, size_t size,
                             bool include_unit = true);

  /**
   * Converts a pressure into a formatted string
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   * @param value the pressure
   * @param unit the pressure unit (e.g. meters, feet, ...)
   * @param include_unit include the unit into the string?
   */
  void FormatPressure(TCHAR *buffer, size_t size, AtmosphericPressure value,
                      Unit unit, bool include_unit = true);

  /**
   * Converts a double-based Pressure into a formatted string
   * @param Pressure The double-based Pressure
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   */
  void FormatUserPressure(AtmosphericPressure value, TCHAR *buffer, size_t size,
                          bool include_unit = true);

  /**
   * Returns the proper format to display the pressure
   * @param unit the pressure unit
   * @return the format
   */
  const TCHAR* GetPressureFormat(Unit unit, bool include_unit = false);

  /**
   * Give the proper format to display the pressure
   * @param buffer buffer string to write to (pointer)
   * @param size Size of the buffer
   * @return True if buffer long enough, False otherwise
   */
  const TCHAR* GetFormatUserPressure(bool include_unit = false);

  /**
   * Give the basic step size for pressure editing
   * @param unit the pressure unit
   * @return the step size
   */
  fixed GetPressureStep(Unit unit);

  /**
   * Give the basic step for pressure editing
   * @return the step
   */
  fixed GetUserPressureStep();
};

#endif
