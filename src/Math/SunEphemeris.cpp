// C program calculating the sunrise and sunset for
// the current date and a fixed location(latitude,longitude)
// Note, twilight calculation gives insufficient accuracy of results
// Jarmo Lammi 1999 - 2001
// Last update July 21st, 2001
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Math/SunEphemeris.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/RoughTime.hpp"

#include <cstdint>

/** Sun radius in degrees (?) */
static constexpr double SUN_DIAMETER = 0.53;

// Atmospheric refraction degrees
static constexpr double AIR_REFRACTION = 34.0 / 60.0;

namespace SunEphemeris {

/**
 * Get the days to J2000
 * FNday only works between 1901 to 2099 - see Meeus chapter 7
 * @param y Year
 * @param m Month
 * @param d Day
 * @param h UT in decimal hours
 * @return days to J2000
 */
static constexpr double
FNday(const BrokenDateTime date_time) noexcept
{
  assert(date_time.IsPlausible());

  const int_fast32_t luku =
    -7 * static_cast<int_fast32_t>(date_time.year + (date_time.month + 9) / 12) / 4 +
    static_cast<int_fast32_t>(275 * date_time.month / 9 + date_time.day) +
    static_cast<int_fast32_t>(date_time.year) * 367;

  return double(luku) - 730531.5 + (date_time.hour % 24) / 24.;
}

/**
 * Calculating the hourangle
 * @param lat Latitude
 * @param declin Declination
 * @return The hourangle
 */
[[gnu::const]]
static Angle
GetHourAngle(Angle lat, Angle declin) noexcept
{
  Angle dfo = Angle::Degrees(SUN_DIAMETER / 2 + AIR_REFRACTION);

  // Correction: different sign at southern hemisphere
  if (lat.IsNegative())
    dfo.Flip();

  auto fo = (declin + dfo).tan() * lat.tan();
  return Angle::asin(fo) + Angle::QuarterCircle();
}

/**
 * Calculating the hourangle for twilight times
 * @param lat Latitude
 * @param declin Declination
 * @return The hourangle for twilight times
 */
[[gnu::const]]
static Angle
GetHourAngleTwilight(Angle lat, Angle declin) noexcept
{
  Angle df1 = Angle::Degrees(6);

  // Correction: different sign at southern hemisphere
  if (lat.IsNegative())
    df1.Flip();

  auto fi = (declin + df1).tan() * lat.tan();
  return Angle::asin(fi) + Angle::QuarterCircle();
}

/**
 * Find the ecliptic longitude of the Sun
 * @return The ecliptic longitude of the Sun
 */
[[gnu::pure]]
static Angle
GetEclipticLongitude(double d, Angle L) noexcept
{
  //   mean anomaly of the Sun
  Angle g = Angle::Degrees(357.528 + .9856003 * d).AsBearing();

  //   Ecliptic longitude of the Sun
  return (Angle::Degrees(1.915) * g.sin() + L +
          Angle::Degrees(.02) * (g * 2).sin()).AsBearing();
}

[[gnu::pure]]
static Angle
GetMeanSunLongitude(double d) noexcept
{
  // mean longitude of the Sun
  return Angle::Degrees(280.461 + .9856474 * d).AsBearing();
}

/**
 * Calculates the sun's azimuth at a given location and time
 * @param Location azimuth at what location
 * @param time azimuth at what time
 * @param dec precalculated declination angle
 * @return sun's azimuth
 * @see http://www.providence.edu/mcs/rbg/java/sungraph.htm
 */
[[gnu::pure]]
static Angle
CalculateAzimuth(const GeoPoint &Location, const BrokenTime &time,
                 const RoughTimeDelta time_zone, const Angle dec) noexcept
{
  assert(time.IsPlausible());

  using Hours = std::chrono::duration<double, std::chrono::hours::period>;

  const auto time_hours =
    std::chrono::duration_cast<Hours>(time.DurationSinceMidnight());
  const auto tz_hours =
    std::chrono::duration_cast<Hours>(time_zone.ToDuration());

  const auto T = (time_hours - Hours{12} + tz_hours).count();
  Angle t = Angle::Degrees(15) * T;

  const auto [latitude_sin, latitude_cos] = Location.latitude.SinCos();
  const auto [dec_sin, dec_cos] = dec.SinCos();
  const auto [t_sin, t_cos] = t.SinCos();

  return -Angle::FromXY(latitude_cos * dec_sin -
                        latitude_sin * dec_cos * t_cos,
                        dec_cos * t_sin);
}

Result
CalcSunTimes(const GeoPoint &location, const BrokenDateTime &date_time,
             const RoughTimeDelta time_zone) noexcept
{
  Result result;

  assert(date_time.IsPlausible());

  auto days_to_j2000 = FNday(date_time);

  Angle l = GetMeanSunLongitude(days_to_j2000);

  // Use GetEclipticLongitude to find the ecliptic longitude of the Sun
  Angle lambda = GetEclipticLongitude(days_to_j2000, l);
  const auto [lambda_sin, lambda_cos] = lambda.SinCos();

  // Obliquity of the ecliptic
  Angle obliquity = Angle::Degrees(23.439 - .0000004 * days_to_j2000);
  const auto [obliquity_sin, obliquity_cos] = obliquity.SinCos();

  // Find the RA and DEC of the Sun
  Angle alpha = Angle::FromXY(lambda_cos, obliquity_cos * lambda_sin);
  Angle delta = Angle::asin(obliquity_sin * lambda_sin);

  // Find the Equation of Time in minutes
  // Correction suggested by David Smith
  Angle ll = l - alpha;
  if (l < Angle::HalfCircle())
    ll += Angle::FullCircle();

  auto equation = 1440 * (1 - ll / Angle::FullCircle());

  Angle hour_angle = GetHourAngle(location.latitude, delta);
  Angle hour_angle_twilight = GetHourAngleTwilight(location.latitude, delta);

  result.azimuth = CalculateAzimuth(location, date_time, time_zone, delta);

  // length of twilight in hours
  auto twilight_hours = (hour_angle_twilight - hour_angle).Hours();

  // Conversion of angle to hours and minutes
  result.day_length = 2 * hour_angle.Hours();

  if (result.day_length < 0.0001)
    // arctic winter
    result.day_length = 0;

  result.time_of_sunrise = 12. - hour_angle.Hours()
    + time_zone.AsMinutes() / 60.
    - location.longitude.Degrees() / 15. + equation / 60.;

  if (result.time_of_sunrise > 24)
    result.time_of_sunrise -= 24;

  result.time_of_sunset = result.time_of_sunrise + result.day_length;
  result.time_of_noon = result.time_of_sunrise + hour_angle.Hours();

  // morning twilight begin
  result.morning_twilight = result.time_of_sunrise - twilight_hours;
  // evening twilight end
  result.evening_twilight = result.time_of_sunset + twilight_hours;

  return result;
}

Angle
CalcAzimuth(const GeoPoint &location, const BrokenDateTime &date_time,
            const RoughTimeDelta time_zone) noexcept
{
  assert(date_time.IsPlausible());

  auto days_to_j2000 = FNday(date_time);

  Angle l = GetMeanSunLongitude(days_to_j2000);

  // Use GetEclipticLongitude to find the ecliptic longitude of the Sun
  Angle lambda = GetEclipticLongitude(days_to_j2000, l);

  // Obliquity of the ecliptic
  Angle obliquity = Angle::Degrees(23.439 - .0000004 * days_to_j2000);

  // Find the DEC of the Sun
  Angle delta = Angle::asin(obliquity.sin() * lambda.sin());

  return CalculateAzimuth(location, date_time, time_zone, delta);
}

} // namespace SunEphemeris
