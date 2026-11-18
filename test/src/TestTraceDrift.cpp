// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Trace/DriftTerms.hpp"
#include "TestUtil.hpp"

using namespace std::chrono;

static TracePoint
MakePoint(unsigned time, unsigned drift_factor) noexcept
{
  return TracePoint(GeoPoint(Angle::Degrees(8), Angle::Degrees(47)),
                    duration<unsigned>{time}, 1000, 0, drift_factor);
}

/* fixes unevenly spaced in time, each with another drift factor */
static const TracePoint points[] = {
  MakePoint(100, 256),
  MakePoint(101, 200),
  MakePoint(104, 128),
  MakePoint(109, 64),
};

static TraceDriftTerms
Terms(unsigned i) noexcept
{
  return TraceDriftTerms::FromTracePoint(points[i]);
}

static void
TestFixes(TimeStamp now)
{
  for (unsigned i = 0; i < 4; ++i)
    ok1(equals(Terms(i).At(now), points[i].CalculateDrift(now)));
}

static void
TestLinear(TimeStamp now)
{
  const double u = 0.3;
  const auto blend = Terms(1) * (1 - u) + Terms(2) * u;

  ok1(equals(blend.At(now),
             points[1].CalculateDrift(now) * (1 - u) +
             points[2].CalculateDrift(now) * u));
}

static void
TestCatmullRom(TimeStamp now)
{
  /* the uniform Catmull-Rom weights at t=0.25 */
  static constexpr double c[] = {
    -0.0703125, 0.8671875, 0.2265625, -0.0234375,
  };

  const auto blend = Terms(0) * c[0] + Terms(1) * c[1] +
    Terms(2) * c[2] + Terms(3) * c[3];

  double expected = 0;
  for (unsigned i = 0; i < 4; ++i)
    expected += points[i].CalculateDrift(now) * c[i];

  ok1(equals(blend.At(now), expected));
}

/**
 * A point half way between two fixes one second apart is half a
 * second younger than the first; a time in whole seconds would give
 * it the drift of the first fix.
 */
static void
TestFraction()
{
  const TimeStamp now{FloatDuration{130}};
  const TracePoint a = MakePoint(120, 256), b = MakePoint(121, 256);
  const auto blend = TraceDriftTerms::FromTracePoint(a) * 0.5 +
    TraceDriftTerms::FromTracePoint(b) * 0.5;

  ok1(equals(a.CalculateDrift(now), 10));
  ok1(equals(b.CalculateDrift(now), 9));
  ok1(equals(blend.At(now), 9.5));
}

int
main()
{
  plan_tests(2 * 6 + 3);

  /* two times: the terms must hold for any "now" */
  for (const double seconds : {110.5, 3600.25}) {
    const TimeStamp now{FloatDuration{seconds}};
    TestFixes(now);
    TestLinear(now);
    TestCatmullRom(now);
  }

  TestFraction();

  return exit_status();
}
