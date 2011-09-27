/*
Copyright_License {

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

#include "ProtectedRoutePlanner.hpp"

void
ProtectedRoutePlanner::SetTerrain(const RasterTerrain *terrain)
{
  ExclusiveLease lease(*this);
  lease->set_terrain(terrain);
}

void
ProtectedRoutePlanner::SetPolars(const GlidePolar &glide_polar,
                                 const GlidePolar &safety_polar,
                                 const SpeedVector &wind)
{
  ExclusiveLease lease(*this);
  lease->update_polar(glide_polar, safety_polar, wind);
}

void
ProtectedRoutePlanner::SolveRoute(const AGeoPoint &dest,
                                  const AGeoPoint &start,
                                  const RoutePlannerConfig &config,
                                  const RoughAltitude h_ceiling,
                                  Route &solution)
{
  ExclusiveLease lease(*this);
  lease->synchronise(airspaces, dest, start);
  lease->solve(dest, start, config, h_ceiling);
  lease->get_solution(solution);
}

bool
ProtectedRoutePlanner::Intersection(const AGeoPoint &origin,
                                    const AGeoPoint &destination,
                                    GeoPoint &intx) const
{
  Lease lease(*this);
  return lease->intersection(origin, destination, intx);
}

void
ProtectedRoutePlanner::SolveReach(const AGeoPoint &origin, const bool do_solve)
{
  ExclusiveLease lease(*this);
  lease->solve_reach(origin, do_solve);
}

void
ProtectedRoutePlanner::AcceptInRange(const GeoBounds &bounds,
                                     TriangleFanVisitor &visitor) const
{
  Lease lease(*this);
  lease->accept_in_range(bounds, visitor);
}
