// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Round-trip and partial-write detection for LXNAV PLXV0 POLAR
// coefficient scaling (#2397).

#include "Device/Driver/LX/LXNAVPolarConversion.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "TestUtil.hpp"

#include <cmath>

[[gnu::const]]
static double
SinkRateMs(const PolarCoefficients &pc, double v_ms) noexcept
{
  return v_ms * (v_ms * pc.a + pc.b) + pc.c;
}

static void
TestRoundTrip(const PolarCoefficients &original)
{
  double a_lx, b_lx, c_lx;
  LXNAVPolar::ToNmeaPolar(original, a_lx, b_lx, c_lx);
  const PolarCoefficients back = LXNAVPolar::FromNmeaPolar(a_lx, b_lx, c_lx);

  ok1(equals(original.a, back.a));
  ok1(equals(original.b, back.b));
  ok1(equals(original.c, back.c));

  /* Same sink at an arbitrary airspeed in both representations */
  constexpr double v_ms = 35.;
  const double v_norm = v_ms / LXNAVPolar::V_REF_MS;
  const double w_si = SinkRateMs(original, v_ms);
  const double w_lx = a_lx * v_norm * v_norm + b_lx * v_norm + c_lx;
  ok1(equals(w_si, w_lx));
}

static void
TestPartialPolarDetection()
{
  /* Pilot-weight-only write used by SetPilotWeight() — empties a,b,c */
  ok1(LXNAVPolar::IsPartialPolarWrite(
        "PLXV0,POLAR,W,,,,,,,,90.00,,"));
  ok1(LXNAVPolar::IsPartialPolarWrite(
        "$PLXV0,POLAR,W,,,,,,,,90.00,,*1A"));

  /* Empty-weight-only write */
  ok1(LXNAVPolar::IsPartialPolarWrite(
        "PLXV0,POLAR,W,,,,,,,265.00,,,"));

  /* Full POLAR write must not be treated as partial */
  ok1(!LXNAVPolar::IsPartialPolarWrite(
        "PLXV0,POLAR,W,1.780,-3.030,1.930,30.0,292,600,265,90,LS 7,0"));
  ok1(!LXNAVPolar::IsPartialPolarWrite(
        "$PLXV0,POLAR,W,1.780,-3.030,1.930,30.0,292,600,265,90,LS 7,0*21"));

  /* Unrelated sentence */
  ok1(!LXNAVPolar::IsPartialPolarWrite("PLXV0,MC,W,1.5"));
}

int
main()
{
  plan_tests(8 + 6);

  /* Polar 1: 206 Hornet sample (PolarStore) */
  TestRoundTrip(PolarCoefficients(0.0022032, -0.08784, 1.47));

  /* Polar 2: different magnitude */
  TestRoundTrip(PolarCoefficients(0.0015, -0.065, 1.1));

  TestPartialPolarDetection();

  return exit_status();
}
