// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * The convection estimate, driven the way GlideComputer drives it: a
 * synthetic sounding fed level by level through UpdateMeasurements().
 *
 * The expected heights are not read back from the code under test;
 * they follow from the construction the code is supposed to perform
 * (WMO-No. 1038 3.3.1.1, WMO/OSTIV 1993 2.3.2) applied to a profile in
 * which every quantity is linear in height, so the crossings can be
 * written down: the thermal index reaches its threshold where the
 * environment and the dry adiabat from the forecast maximum differ by
 * it, and the cloud base is where the dry adiabat meets the dew point.
 */

#include "Atmosphere/CuSonde.hpp"
#include "Atmosphere/DewPoint.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "TestUtil.hpp"

#include <math.h>

/* the sounding */
static constexpr double AIRFIELD_MSL = 400;
static constexpr double FORECAST_MAX_C = 25;
static constexpr double SURFACE_TEMPERATURE_C = 23;
static constexpr double SURFACE_DEWPOINT_C = 8;
/** environmental lapse rate in the mixed layer, K per metre */
static constexpr double MIXED_LAPSE = 0.0094;
/** the dew point falls with height along a mixing-ratio line */
static constexpr double DEWPOINT_LAPSE = 0.0016;
/** top of the mixed layer, above which an inversion warms the air */
static constexpr double MIXED_TOP_AGL = 1800;

static double
EnvironmentCelsius(double h_agl) noexcept
{
  if (h_agl <= MIXED_TOP_AGL)
    return SURFACE_TEMPERATURE_C - MIXED_LAPSE * h_agl;
  return SURFACE_TEMPERATURE_C - MIXED_LAPSE * MIXED_TOP_AGL
    + 0.005 * (h_agl - MIXED_TOP_AGL);
}

static double
DewPointCelsius(double h_agl) noexcept
{
  return SURFACE_DEWPOINT_C - DEWPOINT_LAPSE * h_agl;
}

/**
 * The relative humidity that makes CalculateDewPoint() return the
 * wanted dew point: the Magnus form in DewPoint.hpp inverted.  The
 * formula itself is checked against MetPy in TestDewPoint; here it is
 * only the way to hand a dew point to code that wants a humidity.
 */
static double
HumidityForDewPoint(double t_celsius, double td_celsius) noexcept
{
  const double log_ex = 7.5 * td_celsius / (237.3 + td_celsius);
  return pow(10, log_ex - 7.5 * t_celsius / (237.3 + t_celsius) + 2);
}

enum class Terrain {
  /** the ground stays at the airfield's elevation */
  FLAT,
  /** the ground rises under the glider, half as fast as it climbs */
  RISING,
  /** no terrain file */
  NONE,
};

/**
 * Fly the sounding from below the airfield to 3000 m in 100 m steps.
 */
static void
FlyTheSounding(CuSonde &cu_sonde, Terrain terrain) noexcept
{
  /* value-initialised rather than Reset(): the resets would drag the
     glide polar and task statistics into a test of the sounding */
  NMEAInfo basic{};
  basic.clock = TimeStamp{FloatDuration{1}};

  DerivedInfo calculated{};
  calculated.flight.flying = true;
  calculated.flight.takeoff_time = basic.clock;
  calculated.flight.takeoff_altitude = AIRFIELD_MSL;

  for (double altitude = AIRFIELD_MSL - 100; altitude <= 3000;
       altitude += 100) {
    const double h_agl = altitude - AIRFIELD_MSL;
    const double t = EnvironmentCelsius(h_agl);

    basic.gps_altitude = altitude;
    basic.gps_altitude_available.Update(basic.clock);
    basic.temperature = Temperature::FromCelsius(t);
    basic.temperature_available.Update(basic.clock);
    basic.humidity = HumidityForDewPoint(t, DewPointCelsius(h_agl));
    basic.humidity_available.Update(basic.clock);

    calculated.terrain_valid = terrain != Terrain::NONE;
    calculated.terrain_altitude =
      terrain == Terrain::RISING ? AIRFIELD_MSL + h_agl / 2
      : terrain == Terrain::FLAT ? AIRFIELD_MSL
      : 0;
    calculated.altitude_agl = altitude - calculated.terrain_altitude;
    calculated.altitude_agl_valid = true;

    cu_sonde.UpdateMeasurements(basic, calculated);
  }
}

int
main()
{
  plan_tests(13);

  /* the crossings, from the construction rather than from the code;
     CuSonde::DALR is negative going up */

  /* TI(h) = T_env(h) - T_dry(h)
           = (T_s - MIXED_LAPSE h) - (T_max + DALR h) */
  const double expected_thermal_height_agl =
    (CuSonde::TITHRESHOLD - (SURFACE_TEMPERATURE_C - FORECAST_MAX_C))
    / (-CuSonde::DALR - MIXED_LAPSE);

  /* T_dry(h) = T_d(h): T_max + DALR h = T_ds - DEWPOINT_LAPSE h */
  const double expected_cloud_base_agl =
    (FORECAST_MAX_C - SURFACE_DEWPOINT_C) / (-CuSonde::DALR - DEWPOINT_LAPSE);

  /* both lie where the sounding has data on either side */
  ok1(expected_thermal_height_agl > 0 &&
      expected_thermal_height_agl < 3000 - AIRFIELD_MSL - 100);
  ok1(expected_cloud_base_agl > 0 &&
      expected_cloud_base_agl < 3000 - AIRFIELD_MSL - 100);

  CuSonde cu_sonde;
  cu_sonde.Reset();
  cu_sonde.SetForecastTemperature(Temperature::FromCelsius(FORECAST_MAX_C));
  FlyTheSounding(cu_sonde, Terrain::FLAT);

  /* the ground is where the terrain is, not where the aircraft is
     above it */
  ok1(equals(cu_sonde.ground_height, AIRFIELD_MSL));

  /* the dry adiabat starts at the airfield and falls at DALR: this is
     the assertion that failed while ground_height held the height
     above ground -- dry_temperature was then the same at every level */
  for (unsigned level : {4u, 14u, 24u}) {
    const double h_agl = level * CuSonde::HEIGHT_STEP - AIRFIELD_MSL;
    ok1(fabs(cu_sonde.cslevels[level].dry_temperature.ToCelsius()
             - (FORECAST_MAX_C + CuSonde::DALR * h_agl)) < 0.01);
  }

  /* and so the crossings are where the construction puts them.  The
     profile is linear between levels, so the interpolation is exact
     up to rounding. */
  ok1(fabs(cu_sonde.thermal_height
           - (AIRFIELD_MSL + expected_thermal_height_agl)) < 1);
  ok1(fabs(cu_sonde.cloud_base
           - (AIRFIELD_MSL + expected_cloud_base_agl)) < 1);

  /* changing the forecast afterwards recomputes every level from the
     same ground, and finds the same cloud base again */
  cu_sonde.SetForecastTemperature(Temperature::FromCelsius(FORECAST_MAX_C + 1));
  cu_sonde.SetForecastTemperature(Temperature::FromCelsius(FORECAST_MAX_C));
  ok1(fabs(cu_sonde.cloud_base
           - (AIRFIELD_MSL + expected_cloud_base_agl)) < 1);

  /* The origin is taken once, at the first measurement, and kept:
     with the ground rising under the glider the levels would
     otherwise each get their own adiabat, and the crossing between
     two levels from different adiabats is not the cloud base of
     either.  Same sounding, same answers. */
  CuSonde rising;
  rising.Reset();
  FlyTheSounding(rising, Terrain::RISING);
  ok1(equals(rising.ground_height, AIRFIELD_MSL));
  ok1(fabs(rising.cloud_base - cu_sonde.cloud_base) < 0.01);
  ok1(fabs(rising.thermal_height - cu_sonde.thermal_height) < 0.01);

  /* without a terrain file the take-off altitude stands in for the
     elevation; the caller's altitude_agl is never mistaken for it */
  CuSonde no_terrain;
  no_terrain.Reset();
  FlyTheSounding(no_terrain, Terrain::NONE);
  ok1(equals(no_terrain.ground_height, AIRFIELD_MSL));

  return exit_status();
}
