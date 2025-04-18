// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceIntersectionVisitor.hpp"
#include "AirspaceInterceptSolution.hpp"
#include "AbstractAirspace.hpp"

AirspaceInterceptSolution 
AirspaceIntersectionVisitor::Intercept(const AbstractAirspace &as,
                                       const AircraftState &state,
                                       const AirspaceAircraftPerformance &perf) const
{
  if (intersections.empty())
    return AirspaceInterceptSolution::Invalid();

  AirspaceInterceptSolution solution = AirspaceInterceptSolution::Invalid();
  for (const auto &i : intersections) {
    auto new_solution = as.Intercept(state, perf, i.first, i.second);
    if (new_solution.IsEarlierThan(solution))
      solution = new_solution;
  }

  return solution;
}
