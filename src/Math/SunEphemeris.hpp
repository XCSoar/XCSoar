// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"

struct GeoPoint;
struct BrokenDateTime;
class RoughTimeDelta;

/**
 * Sun ephemeris model, used largely for calculations of sunset times
 * @see http://www.sci.fi/~benefon/azimalt.cpp
 */
namespace SunEphemeris {

struct Result {
  double day_length, morning_twilight, evening_twilight;
  double time_of_noon, time_of_sunset, time_of_sunrise;
  Angle azimuth;
};

/**
 * Calculates all sun-related important times
 * depending on time of year and location
 * @param location Where?
 * @param date_time When?
 * @param time_zone UTC offset for When?
 * @return Attributes of the sun
 */
[[gnu::pure]]
Result
CalcSunTimes(const GeoPoint &location, const BrokenDateTime &date_time,
             RoughTimeDelta time_zone) noexcept;

/**
 * Calculates only the sun's azimuth
 * @param location Where?
 * @param date_time When?
 * @param time_zone UTC offset for When?
 * @return Attributes of the sun
 */
[[gnu::pure]]
Angle
CalcAzimuth(const GeoPoint &location, const BrokenDateTime &date_time,
            RoughTimeDelta time_zone) noexcept;

} // namespace SunEphemeris
