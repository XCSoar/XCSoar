// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Atmosphere/AirDensity.hpp"
#include "Atmosphere/Pressure.hpp"
#include "test_debug.hpp"

#include <stdio.h>

extern "C" {
#include "tap.h"
}

static bool
test_find_qnh()
{
  AtmosphericPressure sp = AtmosphericPressure::Standard().QNHAltitudeToStaticPressure(100);
  AtmosphericPressure pres =
    AtmosphericPressure::FindQNHFromPressure(sp, 100);
  return fabs(pres.GetHectoPascal() - 1013.25) < 0.01;
}

static bool
test_find_qnh2()
{
  AtmosphericPressure sp = AtmosphericPressure::Standard().QNHAltitudeToStaticPressure(100);
  AtmosphericPressure pres =
    AtmosphericPressure::FindQNHFromPressure(sp, 120);
  if (verbose) {
    printf("%g\n", double(pres.GetHectoPascal()));
  }
  return fabs(pres.GetHectoPascal() - 1015.6) < 0.1;
  // example, QNH=1014, ps=100203
  // alt= 100
  // alt_known = 120
  // qnh= 1016
}

static bool
test_qnh_to_static()
{
  AtmosphericPressure pres = AtmosphericPressure::Standard();
  auto p0 = pres.QNHAltitudeToStaticPressure(0).GetPascal();
  if (verbose) {
    printf("%g\n", double(p0));
  }
  return fabs(p0-101325)<0.1;
}

static bool
test_qnh_round()
{
  AtmosphericPressure sp = AtmosphericPressure::Standard().QNHAltitudeToStaticPressure(100);
  AtmosphericPressure pres =
    AtmosphericPressure::FindQNHFromPressure(sp, 120);
  auto h0 = pres.PressureAltitudeToQNHAltitude(100);
  if (verbose) {
    printf("%g\n", double(h0));
  }
  return fabs(h0-120)<1;
}

static bool
test_qnh_round2()
{
  AtmosphericPressure sp = AtmosphericPressure::Standard().QNHAltitudeToStaticPressure(100);
  AtmosphericPressure pres =
    AtmosphericPressure::FindQNHFromPressure(sp, 120);
  auto h0 = pres.StaticPressureToQNHAltitude(pres);
  if (verbose) {
    printf("%g %g\n", double(pres.GetPascal()), double(h0));
  }
  return fabs(h0)<1;
}

static bool
test_isa_pressure(const double alt, const double prat)
{
  const AtmosphericPressure pres = AtmosphericPressure::Standard();
  auto p0 = pres.QNHAltitudeToStaticPressure(alt).GetPascal();
  if (verbose) {
    printf("%g\n", double(p0));
  }
  return fabs(p0/101325-prat)<0.001;
}

static bool
test_isa_density(const double alt, const double prat)
{
  auto p0 = AirDensity(alt);
  if (verbose) {
    printf("%g\n", double(p0));
  }
  return fabs(p0/1.225-prat)<0.001;
}

int main(int argc, char** argv)
{

  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  plan_tests(9);

  ok(test_find_qnh(),"find qnh 0-0",0);
  ok(test_find_qnh2(),"find qnh 100-120",0);

  ok(test_qnh_to_static(),"qnh to static",0);
  ok(test_qnh_round(),"qnh round trip",0);

  ok(test_qnh_round2(),"qnh round 2",0);

  ok(test_isa_pressure(1524, 0.8320), "isa pressure at 1524m",0);
  ok(test_isa_pressure(6096, 0.4594), "isa pressure at 6096m",0);

  ok(test_isa_density(1524, 0.8617), "isa density at 1524m",0);
  ok(test_isa_density(6096, 0.5328), "isa density at 6096m",0);

  return exit_status();
}
