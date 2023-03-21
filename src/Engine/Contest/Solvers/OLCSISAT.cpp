// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OLCSISAT.hpp"
#include "Geo/SearchPointVector.hpp"

OLCSISAT::OLCSISAT(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 6, 1000) {}

/*
  S = total path distance
  V = zig-zag free path distance
  Z = zig-zag portion distance = S-V
  R = remainder distance

  V: 1.00 point per km
  Z: 0.50 points per km
  total score = V + Z*0.50
              = V + 0.50*(S-V)
              = (V + S)/2
*/

ContestResult
OLCSISAT::CalculateResult(const ContestTraceVector &solution) const noexcept
{
  // build convex hull from solution
  SearchPointVector spv;
  for (unsigned i = 0; i < num_stages; ++i)
    spv.emplace_back(solution[i].location);

  spv.PruneInterior();

  // now add leg distances making up the convex hull
  double G = 0;

  if (spv.size() > 1) {
    for (unsigned i = 0; i + 1 < spv.size(); ++i)
      G += spv[i].DistanceTo(spv[i + 1].GetLocation());

    // closing leg (end to start)
    G += spv[spv.size() - 1].DistanceTo(spv[0].GetLocation());
  }

  // R distance (start to end)
  const auto R = solution[0].DistanceTo(solution[num_stages - 1].GetLocation());

  // V zigzag-free distance
  const auto V = G - R;

  // S = total distance
  ContestResult result = ContestDijkstra::CalculateResult(solution);
  result.score = ApplyHandicap((V + result.distance) / 2000);
  return result;
}
