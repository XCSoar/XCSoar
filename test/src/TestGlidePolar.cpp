// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TestUtil.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Units/System.hpp"

#include <cstdio>

class GlidePolarTest
{
  GlidePolar polar;

public:
  void Run();

private:
  void Init();
  void TestBasic();
  void TestBallast();
  void TestBugs();
  void TestMC();
  void TestDensityRatio();
};

void
GlidePolarTest::Init()
{
  // Set operational parameters before any method that calls Update()
  polar.bugs = 1;
  polar.mc = 0;

  // Polar 1 from PolarStore (206 Hornet)
  polar.SetCoefficients(PolarCoefficients(0.0022032, -0.08784,
                                          1.47), false);

  polar.SetReferenceMass(318, false);
  polar.SetEmptyMass(228, false);
  polar.SetCrewMass(90, false);
  polar.SetBallastRatio(100 / polar.reference_mass);

  polar.SetWingArea(9.8);

  // No ballast
  polar.SetBallastLitres(0);

  polar.SetVMax(Units::ToSysUnit(200, Unit::KILOMETER_PER_HOUR), false);
}

void
GlidePolarTest::TestBasic()
{
  polar.Update();

  ok1(equals(polar.polar.a, polar.reference_polar.a));
  ok1(equals(polar.polar.b, polar.reference_polar.b));
  ok1(equals(polar.polar.c, polar.reference_polar.c));

  ok1(equals(polar.SinkRate(Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR)), 0.606));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR)), 0.99));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR)), 1.918));

  ok1(equals(polar.GetSMax(), polar.SinkRate(polar.GetVMax())));

  ok1(equals(polar.GetVMin(), 19.934640523));
  ok1(equals(polar.GetSMin(), polar.SinkRate(polar.GetVMin())));
  ok1(equals(polar.GetVTakeoff(), polar.GetVMin() / 2));

  ok1(equals(polar.GetVBestLD(), 25.830434162));
  ok1(equals(polar.GetSBestLD(), polar.SinkRate(polar.GetVBestLD())));
  ok1(equals(polar.GetBestLD(), polar.GetVBestLD() / polar.GetSBestLD()));

  ok1(equals(polar.GetTotalMass(), 318));
  ok1(equals(polar.GetWingLoading(), 32.448979592));
  ok1(equals(polar.GetBallastFraction(), 0));
  ok1(equals(polar.GetBallastLitres(), 0));
  ok1(polar.IsBallastable());
  ok1(!polar.HasBallast());
}

void
GlidePolarTest::TestBallast()
{
  polar.SetBallastFraction(0.25);

  ok1(equals(polar.GetBallastLitres(), 25));
  ok1(equals(polar.GetBallastFraction(), 0.25));

  polar.SetBallastLitres(50);

  ok1(equals(polar.GetBallastLitres(), 50));
  ok1(equals(polar.GetBallastFraction(), 0.5));
  ok1(equals(polar.GetTotalMass(), 368));
  ok1(equals(polar.GetWingLoading(), 37.551020408));
  ok1(polar.HasBallast());

  double loading_factor = sqrt(polar.GetTotalMass() / polar.reference_mass);
  ok1(equals(polar.polar.a, polar.reference_polar.a / loading_factor));
  ok1(equals(polar.polar.b, polar.reference_polar.b));
  ok1(equals(polar.polar.c, polar.reference_polar.c * loading_factor));

  ok1(equals(polar.SinkRate(Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR)),
             0.640739));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR)),
             0.928976));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR)),
             1.722908));

  ok1(equals(polar.GetVMin(), 21.44464));
  ok1(equals(polar.GetVBestLD(), 27.78703));

  polar.SetBallastLitres(0);
  ok1(!polar.HasBallast());
}

void
GlidePolarTest::TestBugs()
{
  polar.SetBugs(0.75);
  ok1(equals(polar.GetBugs(), 0.75));

  ok1(equals(polar.polar.a, polar.reference_polar.a * 4 / 3));
  ok1(equals(polar.polar.b, polar.reference_polar.b * 4 / 3));
  ok1(equals(polar.polar.c, polar.reference_polar.c * 4 / 3));

  ok1(equals(polar.SinkRate(Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR)),
             0.808));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR)),
             1.32));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR)),
             2.557333));

  ok1(equals(polar.GetVMin(), 19.93464));
  ok1(equals(polar.GetVBestLD(), 25.83043));

  polar.SetBugs(1);
}

void
GlidePolarTest::TestMC()
{
  polar.SetMC(1);
  ok1(equals(polar.GetVBestLD(), 33.482780452));

  polar.SetMC(0);
  ok1(equals(polar.GetVBestLD(), 25.830434162));
}

void
GlidePolarTest::TestDensityRatio()
{
  // Baseline values at sea level (density_ratio == 1.0 by default)
  polar.SetDensityRatio(1.0);
  const double vmin_sl    = polar.GetVMin();
  const double vbld_sl    = polar.GetVBestLD();
  const double sbld_sl    = polar.GetSBestLD();
  const double bestld_sl  = polar.GetBestLD();
  const double sink80_sl  = polar.SinkRate(Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR));

  // Apply a density ratio representing high altitude (e.g. ~3500m MSL -> DR ~1.1)
  const double dr = 1.1;
  polar.SetDensityRatio(dr);

  // All optimal airspeeds scale by DR
  ok1(equals(polar.GetVMin(),    vmin_sl * dr));
  ok1(equals(polar.GetVBestLD(), vbld_sl * dr));

  // Sink rates at corresponding TAS scale by DR
  ok1(equals(polar.GetSBestLD(), sbld_sl * dr));

  // Best L/D ratio (dimensionless) is preserved at altitude
  ok1(equals(polar.GetBestLD(), bestld_sl));

  // SinkRate(V_TAS) at altitude == DR * SinkRate_IAS(V_TAS / DR)
  // i.e. the same IAS as 80 km/h at sea level gives DR times the sea-level sink
  const double v_tas_80ias = Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR) * dr;
  ok1(equals(polar.SinkRate(v_tas_80ias), sink80_sl * dr));

  // SinkRate at the altitude-corrected VBestLD must equal SBestLD
  ok1(equals(polar.SinkRate(polar.GetVBestLD()), polar.GetSBestLD()));

  // Reset to sea level: values must return to baseline
  polar.SetDensityRatio(1.0);
  ok1(equals(polar.GetVMin(),    vmin_sl));
  ok1(equals(polar.GetVBestLD(), vbld_sl));
}

void
GlidePolarTest::Run()
{
  Init();
  TestBasic();
  TestBallast();
  TestBugs();
  TestMC();
  TestDensityRatio();
}

int main()
{
  plan_tests(54);

  GlidePolarTest test;
  test.Run();

  return exit_status();
}
