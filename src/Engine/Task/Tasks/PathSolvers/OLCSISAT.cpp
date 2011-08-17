/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Trace/Trace.hpp"
#include "Navigation/SearchPointVector.hpp"

OLCSISAT::OLCSISAT(const Trace &_trace)
  :ContestDijkstra(_trace, 6, 1000) {}

/*
  S = total path distance
  V = zig-zag free path distance
  Z = zig-zag portion distance = S-V
  R = remainder distance

  V: 1.00 point per km
  Z: 0.75 points per km
  total score = V + Z*0.75
              = V + 0.75*(S-V)
              = (V + 3*S)/4
*/

fixed
OLCSISAT::calc_score() const
{
  // build convex hull from solution
  SearchPointVector spv;
  for (unsigned i = 0; i < num_stages; ++i)
    spv.push_back(solution[i]);

  prune_interior(spv);

  // now add leg distances making up the convex hull
  fixed G = fixed_zero;

  if (spv.size() > 1) {
    for (unsigned i = 0; i + 1 < spv.size(); ++i)
      G += spv[i].distance(spv[i + 1].get_location());

    // closing leg (end to start)
    G += spv[spv.size() - 1].distance(spv[0].get_location());
  }

  // R distance (start to end)
  const fixed R = solution[0].distance(solution[num_stages - 1].get_location());

  // V zigzag-free distance
  const fixed V = G - R;

  // S = total distance
  const fixed S = calc_distance();

  return apply_handicap((V + fixed(3) * S) * fixed(0.00025));
}

