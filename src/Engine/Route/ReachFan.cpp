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

#include "ReachFan.hpp"
#include "Terrain/RasterMap.hpp"
#include "ReachFanParms.hpp"
#include "ReachResult.hpp"

static constexpr int MIN_FLOOR_CLEARANCE = 100;

void
ReachFan::Reset()
{
  root.Clear();
  terrain_base = 0;
}

bool
ReachFan::Solve(const AGeoPoint origin, const RoutePolars &rpolars,
                const RasterMap* terrain, const bool do_solve)
{
  Reset();

  // initialise projection
  projection = FlatProjection(origin);

  const auto h = terrain
    ? terrain->GetHeight(origin)
    : TerrainHeight::Invalid();
  const int h2 = h.GetValueOr0();

  ReachFanParms parms(rpolars, projection, terrain_base, terrain);
  const AFlatGeoPoint ao(projection.ProjectInteger(origin), origin.altitude);

  // immediate exit if starting below terrain, or starting below floor
  // with some clearance (not worth scanning if too close)
  if ((!h.IsInvalid() &&
      (origin.altitude <= h2 + rpolars.GetSafetyHeight()))
      || (origin.altitude < MIN_FLOOR_CLEARANCE + rpolars.GetFloor() + rpolars.GetSafetyHeight())) {
    terrain_base = h2;
    root.DummyReach(ao);
    return false;
  }

  if (do_solve)
    root.FillReach(ao, parms);
  else
    root.DummyReach(ao);

  if (!h.IsInvalid()) {
    parms.terrain_base = h2;
    parms.terrain_counter = 1;
  } else {
    parms.terrain_base = 0;
    parms.terrain_counter = 0;
  }

  if (parms.terrain)
    root.UpdateTerrainBase(ao, parms);

  terrain_base = parms.terrain_base;
  return true;
}

bool
ReachFan::FindPositiveArrival(const AGeoPoint dest, const RoutePolars &rpolars,
                              ReachResult &result_r) const
{
  if (root.IsEmpty())
    return false;

  const FlatGeoPoint d(projection.ProjectInteger(dest));
  const ReachFanParms parms(rpolars, projection, terrain_base);

  result_r.Clear();

  // first calculate direct (terrain-independent height)
  result_r.direct = root.DirectArrival(d, parms);

  if (root.IsDummy())
    /* terrain reach is not available, stop here */
    return true;

  // if can't reach even with no terrain, exit early
  if (std::min(root.GetHeight(), result_r.direct) < dest.altitude) {
    result_r.terrain = result_r.direct;
    result_r.terrain_valid = ReachResult::Validity::UNREACHABLE;
    return true;
  }

  // now calculate turning solution
  result_r.terrain = dest.altitude - 1;
  result_r.terrain_valid = root.FindPositiveArrival(d, parms, result_r.terrain)
    ? ReachResult::Validity::VALID
    : ReachResult::Validity::UNREACHABLE;

  return true;
}

void
ReachFan::AcceptInRange(const GeoBounds &bounds,
                        FlatTriangleFanVisitor &visitor) const
{
  if (root.IsEmpty())
    return;

  const FlatBoundingBox bb = projection.Project(bounds);
  root.AcceptInRange(bb, visitor);
}
