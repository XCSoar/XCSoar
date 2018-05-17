/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "OLCSISAT.hpp"
#include "Geo/SearchPointVector.hpp"

OLCSISAT::OLCSISAT(const Trace &_trace)
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
OLCSISAT::CalculateResult(const ContestTraceVector &solution) const
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
