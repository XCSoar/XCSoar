// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace/SpokenAlert.hpp"
#include "TestUtil.hpp"

#include <cstring>

static AirspaceInterceptSolution
Solution(double distance, double altitude) noexcept
{
  AirspaceInterceptSolution solution{};
  solution.distance = distance;
  solution.altitude = altitude;
  solution.elapsed_time = FloatDuration{10};
  return solution;
}

static bool
ClipsAre(AirspaceWarning::State state,
         const AirspaceInterceptSolution &solution,
         double nav_altitude, const char *expected) noexcept
{
  const auto clips =
    SpokenAirspaceAlert::From(state, solution, nav_altitude).GetClips();
  return clips.size() == 1 && strcmp(clips[0], expected) == 0;
}

int
main()
{
  plan_tests(6);

  const auto horizontal = Solution(1200, 1500);
  const auto overhead = Solution(0, 1800);
  const auto underneath = Solution(0, 800);

  ok1(ClipsAre(AirspaceWarning::WARNING_INSIDE, overhead, 1000,
               "IDR_WAV_AIRSPACE_INSIDE"));
  ok1(ClipsAre(AirspaceWarning::WARNING_GLIDE, horizontal, 1000,
               "IDR_WAV_AIRSPACE_NEAR"));
  ok1(ClipsAre(AirspaceWarning::WARNING_FILTER, horizontal, 1000,
               "IDR_WAV_AIRSPACE_NEAR"));
  ok1(ClipsAre(AirspaceWarning::WARNING_TASK, horizontal, 1000,
               "IDR_WAV_AIRSPACE_NEAR"));
  ok1(ClipsAre(AirspaceWarning::WARNING_GLIDE, overhead, 1000,
               "IDR_WAV_AIRSPACE_ABOVE"));
  ok1(ClipsAre(AirspaceWarning::WARNING_GLIDE, underneath, 1000,
               "IDR_WAV_AIRSPACE_NEAR"));

  return exit_status();
}
