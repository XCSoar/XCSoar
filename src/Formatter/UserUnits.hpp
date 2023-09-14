// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Units/Unit.hpp"
#include "util/StringBuffer.hxx"
#include "util/Compiler.h"

#include <tchar.h>

class AtmosphericPressure;

/**
 * Converts a double-based wing loading into a formatted string
 * @param value The double-based wing loading
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatUserWingLoading(double value, TCHAR *buffer,
                      bool include_unit = true);

/**
 * Converts a double-based mass into a formatted string
 * @param value The double-based mass
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatUserMass(double value, TCHAR *buffer,
               bool include_unit = true);

/**
 * Converts a double-based Altitude into a formatted string
 * @param Altitude The double-based Altitude
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatUserAltitude(double value, TCHAR *buffer,
                   bool include_unit = true);

[[gnu::const]]
static inline BasicStringBuffer<TCHAR, 32>
FormatUserAltitude(double value)
{
  BasicStringBuffer<TCHAR, 32> buffer;
  FormatUserAltitude(value, buffer.data());
  return buffer;
}

/**
 * Converts a double-based Altitude into a formatted string of the alternate
 * altitude format
 * @param Altitude The double-based Altitude
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatAlternateUserAltitude(double value, TCHAR *buffer,
                            bool include_unit = true);

/**
 * Converts a double-based Arrival Altitude into a formatted string
 * @param Altitude The double-based Arrival Altitude
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatRelativeUserAltitude(double value, TCHAR *buffer,
                           bool include_unit = true);

/**
 * Converts a distance into a formatted string
 * @param value the distance
 * @param buffer buffer string to write to (pointer)
 * @param include_unit include the unit into the string?
 * @param precision the number of decimal places
 */
void
FormatUserDistance(double value, TCHAR *buffer,
                   bool include_unit = true, int precision = 0);

/**
 * Converts a distance into a formatted string using the smaller version
 * of the user-defined distance unit (km -> m, nm -> ft, sm -> ft)
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param value the distance
 * @param include_unit include the unit into the string?
 * @param precision the number of decimal places
 * @return the unit used for output formatting
 */
Unit
FormatSmallUserDistance(TCHAR *buffer, double value,
                        bool include_unit = true, int precision = 0);

/**
 * Converts a double-based horizontal Distance into a formatted string
 * @param Distance The double-based Distance
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
Unit
FormatUserDistanceSmart(double value, TCHAR *buffer,
                        bool include_unit = true,
                        double small_unit_threshold = 0,
                        double precision_threshold = 100);

[[gnu::const]]
static inline BasicStringBuffer<TCHAR, 32>
FormatUserDistanceSmart(double value)
{
  BasicStringBuffer<TCHAR, 32> buffer;
  FormatUserDistanceSmart(value, buffer.data());
  return buffer;
}

Unit
FormatUserMapScale(double value, TCHAR *buffer,
                   bool include_unit = true);

/**
 * Converts a double-based Speed into a formatted string
 * @param Speed The double-based Speed
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @return True if buffer long enough, False otherwise
 */
void
FormatUserSpeed(double value, TCHAR *buffer,
                bool include_unit = true, bool Precision = true);

[[gnu::const]]
static inline BasicStringBuffer<TCHAR, 32>
FormatUserSpeed(double value, bool precision=true)
{
  BasicStringBuffer<TCHAR, 32> buffer;
  FormatUserSpeed(value, buffer.data(), true, precision);
  return buffer;
}

/**
 * Converts a double-based Speed into a formatted string
 * @param Speed The double-based Speed
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @return True if buffer long enough, False otherwise
 */
void
FormatUserWindSpeed(double value, TCHAR *buffer,
                    bool include_unit = true, bool Precision = true);

/**
 * Convert a speed [m/s] to the user's task speed and format it into
 * a string.
 *
 * @param value the speed value [m/s]
 */
void
FormatUserTaskSpeed(double value, TCHAR *buffer,
                    bool include_unit=true, bool precision=true);

[[gnu::const]]
static inline BasicStringBuffer<TCHAR, 32>
FormatUserTaskSpeed(double value, bool precision=true)
{
  BasicStringBuffer<TCHAR, 32> buffer;
  FormatUserTaskSpeed(value, buffer.data(), true, precision);
  return buffer;
}

/**
 * Give the proper format to display the vertical speed
 * @param include_unit include the unit into the string?
 * @param include_sign include the sign into the string?
 * @return the format
 */
const TCHAR* GetUserVerticalSpeedFormat(bool include_unit = false,
                                        bool include_sign = true);

/**
 * Give the basic step for pressure editing
 * @return the step
 */
double
GetUserVerticalSpeedStep();

/**
 * Converts a double-based vertical Speed into a formatted string
 * @param Speed The double-based vertical Speed
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param include_sign include the sign into the string?
 * @return True if buffer long enough, False otherwise
 */
void
FormatUserVerticalSpeed(double value, TCHAR *buffer,
                        bool include_unit = true, bool include_sign = true);

/**
 * Converts a temperature into a formatted string
 * @param temperature The double-based vertical Speed
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatUserTemperature(double value, TCHAR *buffer,
                      bool include_unit = true);

/**
 * Converts a double-based Pressure into a formatted string
 * @param Pressure The double-based Pressure
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void FormatUserPressure(AtmosphericPressure value, TCHAR *buffer,
                        bool include_unit = true);

/**
 * Give the proper format to display the pressure
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @return True if buffer long enough, False otherwise
 */
const TCHAR* GetUserPressureFormat(bool include_unit = false);

/**
 * Give the basic step for pressure editing
 * @return the step
 */
double
GetUserPressureStep();
