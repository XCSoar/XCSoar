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

#include "RoutePlannerGlue.hpp"
#include "Thread/Guard.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Navigation/SpeedVector.hpp"
#include "NMEA/Derived.hpp"
#include <assert.h>

RoutePlannerGlue::RoutePlannerGlue(const Airspaces& master):
  terrain(NULL),
  m_planner(master)
{
}

void
RoutePlannerGlue::set_terrain(const RasterTerrain *_terrain)
{
  terrain = _terrain;
  if (terrain) {
    RasterTerrain::Lease lease(*terrain);
    m_planner.Reset();
    m_planner.SetTerrain(&terrain->map);
  } else {
    m_planner.Reset();
    m_planner.SetTerrain(NULL);
  }
}

bool
RoutePlannerGlue::solve(const AGeoPoint& origin,
                        const AGeoPoint& destination,
                        const RoutePlannerConfig& config,
                        const RoughAltitude h_ceiling)
{
  RasterTerrain::Lease lease(*terrain);
  return m_planner.Solve(origin, destination, config, h_ceiling);
}

void
RoutePlannerGlue::solve_reach(const AGeoPoint &origin,
                              const RoutePlannerConfig &config,
                              const RoughAltitude h_ceiling, const bool do_solve)
{
  if (terrain) {
    RasterTerrain::Lease lease(*terrain);
    m_planner.SolveReach(origin, config, h_ceiling, do_solve);
  } else {
    m_planner.SolveReach(origin, config, h_ceiling, do_solve);
  }
}

bool
RoutePlannerGlue::find_positive_arrival(const AGeoPoint& dest,
                                        RoughAltitude &arrival_height_reach,
                                        RoughAltitude &arrival_height_direct) const
{
  return m_planner.FindPositiveArrival(dest, arrival_height_reach, arrival_height_direct);
}

void
RoutePlannerGlue::accept_in_range(const GeoBounds& bounds,
                                  TriangleFanVisitor& visitor) const
{
  m_planner.AcceptInRange(bounds, visitor);
}

bool
RoutePlannerGlue::intersection(const AGeoPoint& origin,
                               const AGeoPoint& destination,
                               GeoPoint& intx) const
{
  RasterTerrain::Lease lease(*terrain);
  return m_planner.Intersection(origin, destination, intx);
}

RoughAltitude
RoutePlannerGlue::get_terrain_base() const
{
  return m_planner.GetTerrainBase();
}
