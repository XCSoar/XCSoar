// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Terrain/RasterTraits.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(15);

  ok1(RasterTraits::STEP_BITS < RasterTraits::OVERVIEW_BITS);
  ok1(RasterTraits::ToStep(0) == 0);
  ok1(RasterTraits::ToStep(4) == 1);
  ok1(RasterTraits::ToStep(5) == 1);
  ok1(RasterTraits::ToStepCeil(1) == 1);
  ok1(RasterTraits::ToStepCeil(4) == 1);
  ok1(RasterTraits::ToStepCeil(5) == 2);
  ok1(RasterTraits::ToOverview(16) == 1);
  ok1(RasterTraits::SubsamplePhase(RasterTraits::STEP_BITS) == 2);
  ok1(RasterTraits::SubsamplePhase(RasterTraits::OVERVIEW_BITS) == 8);

  ok1(RasterTraits::StepCoversPixel(500, 500));
  ok1(RasterTraits::StepCoversPixel(800, 500));
  ok1(!RasterTraits::StepCoversPixel(499, 500));
  ok1(!RasterTraits::StepCoversPixel(500, 0));
  ok1(!RasterTraits::StepCoversPixel(0, 500));

  return exit_status();
}
