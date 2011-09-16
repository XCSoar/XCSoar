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

#include "ReachFan.hpp"
#include "Route/RoutePolar.hpp"
#include "Terrain/RasterMap.hpp"
#include "ReachFanParms.hpp"

void ReachFan::reset() {
  root.Clear();
  terrain_base = 0;
}

bool ReachFan::solve(const AGeoPoint origin,
                     const RoutePolars &rpolars,
                     const RasterMap* terrain,
                     const bool do_solve) {
  reset();

  // initialise task_proj
  task_proj.reset(origin);
  task_proj.update_fast();

  const short h = terrain
    ? terrain->GetHeight(origin)
    : RasterBuffer::TERRAIN_INVALID;
  const short h2 = RasterBuffer::is_special(h) ? 0 : h;

  ReachFanParms parms(rpolars, task_proj, terrain_base, terrain);
  const AFlatGeoPoint ao(task_proj.project(origin), origin.altitude);

  if (!RasterBuffer::is_invalid(h) &&
      (origin.altitude <= h2 + rpolars.safety_height())) {
    terrain_base = h2;
    root.DummyReach(ao);
    return false;
  }

  if (do_solve) {
    root.FillReach(ao, parms);
  } else {
    root.DummyReach(ao);
  }

  if (!RasterBuffer::is_invalid(h)) {
    parms.terrain_base = h2;
    parms.terrain_counter = 1;
  } else {
    parms.terrain_base = 0;
    parms.terrain_counter = 0;
  }
  if (parms.terrain) {
    root.UpdateTerrainBase(ao, parms);
  }
  terrain_base = parms.terrain_base;
  return true;
}

bool
ReachFan::is_inside(const GeoPoint origin, const bool turning) const
{
  // no data? probably not solved yet
  if (root.empty())
    return false;
  const FlatGeoPoint p = task_proj.project(origin);
  return root.IsInsideTree(p, turning);
}

bool
ReachFan::find_positive_arrival(const AGeoPoint dest,
                                const RoutePolars &rpolars,
                                short& arrival_height_reach,
                                short& arrival_height_direct) const
{
  arrival_height_reach = -1;
  arrival_height_direct = -1;

  if (root.empty())
    return true;

  const FlatGeoPoint d (task_proj.project(dest));
  const ReachFanParms parms(rpolars, task_proj, terrain_base);

  // first calculate direct (terrain-independent height)

  arrival_height_direct = root.DirectArrival(d, parms);

  // if can't reach even with no terrain, exit early

  if (std::min(root.get_height(), arrival_height_direct) < dest.altitude) {
    arrival_height_reach = arrival_height_direct;
    return true;
  }

  // now calculate turning solution

  arrival_height_reach = dest.altitude-1;
  root.FindPositiveArrival(d, parms, arrival_height_reach);

  return true;
}

void
ReachFan::accept_in_range(const GeoBounds& bounds,
                          TriangleFanVisitor& visitor) const
{
  if (root.empty())
    return;

  const FlatBoundingBox bb = task_proj.project(bounds);
  root.AcceptInRange(bb, task_proj, visitor);
}
