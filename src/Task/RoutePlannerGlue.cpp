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

#include "RoutePlannerGlue.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Airspace/ActivePredicate.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"

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
    planner.SetTerrain(nullptr);
  }
}

void
RoutePlannerGlue::Synchronise(const Airspaces &master,
                              const ProtectedAirspaceWarningManager *warnings,
                              const AGeoPoint &origin,
                              const AGeoPoint &destination)
{
  /* ignore acked airspaces (if we have an AirspaceWarningManager) */
  const auto predicate =
    WrapAirspacePredicate(ActiveAirspacePredicate(warnings));

  planner.Synchronise(master, predicate, origin, destination);
}

bool
RoutePlannerGlue::Solve(const AGeoPoint &origin,
                        const AGeoPoint &destination,
                        const RoutePlannerConfig &config,
                        const int h_ceiling)
{
  RasterTerrain::Lease lease(*terrain);
  return planner.Solve(origin, destination, config, h_ceiling);
}

void
RoutePlannerGlue::SolveReach(const AGeoPoint &origin,
                              const RoutePlannerConfig &config,
                              const int h_ceiling, const bool do_solve)
{
  if (terrain) {
    RasterTerrain::Lease lease(*terrain);
    planner.SolveReachTerrain(origin, config, h_ceiling, do_solve);
    planner.SolveReachWorking(origin, config, h_ceiling, do_solve);
  } else {
    planner.SolveReachTerrain(origin, config, h_ceiling, do_solve);
    planner.SolveReachWorking(origin, config, h_ceiling, do_solve);
  }
}

bool
RoutePlannerGlue::FindPositiveArrival(const AGeoPoint &dest,
                                      ReachResult &result_r) const
{
  return planner.FindPositiveArrival(dest, result_r);
}

GeoPoint
RoutePlannerGlue::Intersection(const AGeoPoint &origin,
                               const AGeoPoint &destination) const
{
  RasterTerrain::Lease lease(*terrain);
  return planner.Intersection(origin, destination);
}

int
RoutePlannerGlue::GetTerrainBase() const
{
  return planner.GetTerrainBase();
}
