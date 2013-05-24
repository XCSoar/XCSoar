/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "RoutePlannerGlue.hpp"
#include "Terrain/RasterTerrain.hpp"

void
RoutePlannerGlue::SetTerrain(const RasterTerrain *_terrain)
{
  terrain = _terrain;
  if (terrain) {
    RasterTerrain::Lease lease(*terrain);
    planner.Reset();
    planner.SetTerrain(&terrain->map);
  } else {
    planner.Reset();
    planner.SetTerrain(NULL);
  }
}

void
RoutePlannerGlue::Synchronise(const Airspaces &master,
                              const AGeoPoint &origin,
                              const AGeoPoint &destination)
{
  AirspacePredicateTrue true_predicate;
  const AirspacePredicate &predicate = true_predicate;

  planner.Synchronise(master, predicate, origin, destination);
}

bool
RoutePlannerGlue::Solve(const AGeoPoint &origin,
                        const AGeoPoint &destination,
                        const RoutePlannerConfig &config,
                        const RoughAltitude h_ceiling)
{
  RasterTerrain::Lease lease(*terrain);
  return planner.Solve(origin, destination, config, h_ceiling);
}

void
RoutePlannerGlue::SolveReach(const AGeoPoint &origin,
                              const RoutePlannerConfig &config,
                              const RoughAltitude h_ceiling, const bool do_solve)
{
  if (terrain) {
    RasterTerrain::Lease lease(*terrain);
    planner.SolveReach(origin, config, h_ceiling, do_solve);
  } else {
    planner.SolveReach(origin, config, h_ceiling, do_solve);
  }
}

bool
RoutePlannerGlue::FindPositiveArrival(const AGeoPoint &dest,
                                      ReachResult &result_r) const
{
  return planner.FindPositiveArrival(dest, result_r);
}

void
RoutePlannerGlue::AcceptInRange(const GeoBounds &bounds,
                                  TriangleFanVisitor &visitor) const
{
  planner.AcceptInRange(bounds, visitor);
}

bool
RoutePlannerGlue::Intersection(const AGeoPoint &origin,
                               const AGeoPoint &destination,
                               GeoPoint &intx) const
{
  RasterTerrain::Lease lease(*terrain);
  return planner.Intersection(origin, destination, intx);
}

RoughAltitude
RoutePlannerGlue::GetTerrainBase() const
{
  return planner.GetTerrainBase();
}
