// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * The expected dew points are not computed with the same formula they
 * check: they come from MetPy 1.7.1
 * (metpy.calc.dewpoint_from_relative_humidity), so the two sides are
 * independent.  The Magnus approximation in DewPoint.hpp stays within
 * 0.05 K of them over this table and within 0.21 K over the whole
 * range -10..35 degC and 10..99 % relative humidity.
 */

#include "Atmosphere/CuSonde.hpp"
#include "Atmosphere/DewPoint.hpp"
#include "TestUtil.hpp"

#include <math.h>

struct Reference {
  double temperature_celsius, humidity_percent, dewpoint_celsius;
};

static constexpr Reference REFERENCE[] = {
  {  30.0,  90.0,  28.1363 },
  {  20.0,  50.0,   9.2561 },
  {  15.0, 100.0,  14.9914 },
  {  10.0,  30.0,  -6.7964 },
  {   0.0,  60.0,  -6.8511 },
  { -10.0,  80.0, -12.8117 },
  {  35.0,  20.0,   8.6673 },
};

int
main()
{
  plan_tests(14);

  for (const auto &i : REFERENCE) {
    const auto dewpoint =
      CalculateDewPoint(Temperature::FromCelsius(i.temperature_celsius),
                        i.humidity_percent);
    ok1(fabs(dewpoint.ToCelsius() - i.dewpoint_celsius) < 0.25);
  }

  /* saturated air has its dew point at the temperature */
  ok1(fabs(CalculateDewPoint(Temperature::FromCelsius(12), 100).ToCelsius()
           - 12) < 0.01);

  /* the dew point never exceeds the temperature, and falls as the air
     dries */
  const auto t = Temperature::FromCelsius(20);
  ok1(CalculateDewPoint(t, 100) <= t);
  ok1(CalculateDewPoint(t, 40) < CalculateDewPoint(t, 80));

  /* Above saturation the result is unphysical but finite, which is why
     the caller needs a range check and not merely a zero check. */
  ok1(CalculateDewPoint(t, 120) > t);

  /* and the check itself, where it sits: a level offered an
     implausible humidity keeps no dew point, so FindCloudBase() skips
     it.  The temperature of that sample is still recorded -- only the
     humidity was unusable. */
  CuSonde::Level zero{};
  zero.UpdateTemps(true, 0, t);
  ok1(zero.dewpoint_empty());

  CuSonde::Level supersaturated{};
  supersaturated.UpdateTemps(true, 120, t);
  ok1(supersaturated.dewpoint_empty());

  CuSonde::Level good{};
  good.UpdateTemps(true, 60, t);
  ok1(!good.dewpoint_empty());

  /* The other end of the domain cannot be asserted here.  At zero
     humidity the formula takes log10(0) and the result is NaN, but
     every build gets -ffast-math (build/debug.mk), and -ffinite-math-only
     is only switched off again for clang -- so under GCC the compiler
     may assume that NaN does not occur and isfinite() is not to be
     trusted.  That is precisely why the value has to be rejected before
     it reaches this function, in CuSonde::Level::UpdateTemps(), rather
     than detected afterwards. */

  return exit_status();
}
