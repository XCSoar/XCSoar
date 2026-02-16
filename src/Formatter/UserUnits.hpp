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
FormatUserWingLoading(double value, char *buffer,
                      bool include_unit = true) noexcept;

/**
 * Converts a double-based mass into a formatted string
 * @param value The double-based mass
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatUserMass(double value, char *buffer,
               bool include_unit = true) noexcept;

/**
 * Converts a double-based Altitude into a formatted string
 * @param Altitude The double-based Altitude
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatUserAltitude(double value, char *buffer,
                   bool include_unit = true) noexcept;

[[gnu::const]]
static inline auto
FormatUserAltitude(double value) noexcept
{
  BasicStringBuffer<char, 32> buffer;
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
FormatAlternateUserAltitude(double value, char *buffer,
                            bool include_unit = true) noexcept;

/**
 * Converts a double-based Arrival Altitude into a formatted string
 * @param Altitude The double-based Arrival Altitude
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatRelativeUserAltitude(double value, char *buffer,
                           bool include_unit = true) noexcept;

/**
 * Converts a distance into a formatted string
 * @param value the distance
 * @param buffer buffer string to write to (pointer)
 * @param include_unit include the unit into the string?
 * @param precision the number of decimal places
 */
void
FormatUserDistance(double value, char *buffer,
                   bool include_unit = true, int precision = 0) noexcept;

[[gnu::const]]
static inline auto
FormatUserDistance(double value,
                   bool include_unit = true, int precision = 0) noexcept
{
  BasicStringBuffer<char, 32> buffer;
  FormatUserDistance(value, buffer.data(), include_unit, precision);
  return buffer;
}

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
FormatSmallUserDistance(char *buffer, double value,
                        bool include_unit = true, int precision = 0) noexcept;

/**
 * Converts a double-based horizontal Distance into a formatted string
 * @param Distance The double-based Distance
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
Unit
FormatUserDistanceSmart(double value, char *buffer,
                        bool include_unit = true,
                        double small_unit_threshold = 0,
                        double precision_threshold = 100) noexcept;

[[gnu::const]]
static inline auto
FormatUserDistanceSmart(double value, bool include_unit = true,
                        double small_unit_threshold = 0,
                        double precision_threshold = 100) noexcept
{
  BasicStringBuffer<char, 32> buffer;
  FormatUserDistanceSmart(value, buffer.data(), include_unit,
                          small_unit_threshold, precision_threshold);
  return buffer;
}

Unit
FormatUserMapScale(double value, char *buffer,
                   bool include_unit = true) noexcept;

/**
 * Converts a double-based Speed into a formatted string
 * @param Speed The double-based Speed
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @return True if buffer long enough, False otherwise
 */
void
FormatUserSpeed(double value, char *buffer,
                bool include_unit = true, bool Precision = true) noexcept;

[[gnu::const]]
static inline auto
FormatUserSpeed(double value, bool precision=true) noexcept
{
  BasicStringBuffer<char, 32> buffer;
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
FormatUserWindSpeed(double value, char *buffer,
                    bool include_unit = true, bool Precision = true) noexcept;

[[gnu::const]]
static inline auto
FormatUserWindSpeed(double value, bool include_unit = true, bool precision=true) noexcept
{
  BasicStringBuffer<char, 32> buffer;
  FormatUserWindSpeed(value, buffer.data(), include_unit, precision);
  return buffer;
}

/**
 * Convert a speed [m/s] to the user's task speed and format it into
 * a string.
 *
 * @param value the speed value [m/s]
 */
void
FormatUserTaskSpeed(double value, char *buffer,
                    bool include_unit=true, bool precision=true) noexcept;

[[gnu::const]]
static inline auto
FormatUserTaskSpeed(double value, bool precision=true) noexcept
{
  BasicStringBuffer<char, 32> buffer;
  FormatUserTaskSpeed(value, buffer.data(), true, precision);
  return buffer;
}

/**
 * Give the proper format to display the vertical speed
 * @param include_unit include the unit into the string?
 * @param include_sign include the sign into the string?
 * @return the format
 */
const char *
GetUserVerticalSpeedFormat(bool include_unit = false,
                           bool include_sign = true) noexcept;

/**
 * Give the basic step for pressure editing
 * @return the step
 */
double
GetUserVerticalSpeedStep() noexcept;

/**
 * Converts a double-based vertical Speed into a formatted string
 * @param Speed The double-based vertical Speed
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @param include_sign include the sign into the string?
 * @return True if buffer long enough, False otherwise
 */
void
FormatUserVerticalSpeed(double value, char *buffer,
                        bool include_unit = true, bool include_sign = true) noexcept;

[[gnu::const]]
static inline auto
FormatUserVerticalSpeed(double value, bool include_unit = true, bool include_sign = true) noexcept
{
  BasicStringBuffer<char, 32> buffer;
  FormatUserVerticalSpeed(value, buffer.data(), include_unit, include_sign);
  return buffer;
}

/**
 * Converts a temperature into a formatted string
 * @param temperature The double-based vertical Speed
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void
FormatUserTemperature(double value, char *buffer,
                      bool include_unit = true) noexcept;

/**
 * Converts a double-based Pressure into a formatted string
 * @param Pressure The double-based Pressure
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 */
void FormatUserPressure(AtmosphericPressure value, char *buffer,
                        bool include_unit = true) noexcept;

/**
 * Give the proper format to display the pressure
 * @param buffer buffer string to write to (pointer)
 * @param size Size of the buffer
 * @return True if buffer long enough, False otherwise
 */
const char *
GetUserPressureFormat(bool include_unit = false) noexcept;

/**
 * Give the basic step for pressure editing
 * @return the step
 */
double
GetUserPressureStep() noexcept;
