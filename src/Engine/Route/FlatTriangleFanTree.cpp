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

#include "FlatTriangleFanTree.hpp"
#include "RouteLink.hpp"
#include "Terrain/RasterMap.hpp"
#include "ReachFanParms.hpp"
#include "Util/GlobalSliceAllocator.hpp"
#include "Geo/Flat/FlatProjection.hpp"

#define REACH_BUFFER 1
#define REACH_SWEEP (ROUTEPOLAR_Q1-REACH_BUFFER)

#define REACH_MAX_DEPTH 4
#define REACH_MIN_STEP 25
#define REACH_MAX_VERTICES 2000

static bool
AlmostTheSame(const FlatGeoPoint p1, const FlatGeoPoint p2)
{
  const FlatGeoPoint k = p1 - p2;
  const int dmax = std::max(abs(k.x), abs(k.y));
  return dmax <= 1;
}

static bool
TooClose(const FlatGeoPoint p1, const FlatGeoPoint p2)
{
  const FlatGeoPoint k = p1 - p2;
  const int dmax = std::max(abs(k.x), abs(k.y));
  return dmax < REACH_MIN_STEP;
}

void
FlatTriangleFanTree::CalcBB()
{
  FlatTriangleFan::CalcBoundingBox();

  bb_children = bounding_box;

  for (auto &child : children) {
    child.CalcBB();
    bb_children.Merge(child.bb_children);
  }
}

void
FlatTriangleFanTree::FillReach(const AFlatGeoPoint &origin,
                               ReachFanParms &parms)
{
  gaps_filled = false;

  FillReach(origin, 0, ROUTEPOLAR_POINTS, parms);

  for (parms.set_depth = 0; parms.set_depth < REACH_MAX_DEPTH;
      ++parms.set_depth)
    if (!FillDepth(origin, parms))
      // stop searching
      break;

  // this boundingbox update visits the tree recursively
  CalcBB();
}

void
FlatTriangleFanTree::DummyReach(const AFlatGeoPoint &ao)
{
  assert(children.empty());

  AddOrigin(ao, 0);
  CalcBB();
}

bool
FlatTriangleFanTree::FillDepth(const AFlatGeoPoint &origin,
                               ReachFanParms &parms)
{
  if (depth == parms.set_depth) {
    if (gaps_filled)
      return true;
    gaps_filled = true;

    if (parms.vertex_counter > REACH_MAX_VERTICES)
      return false;
    if (parms.fan_counter > REACH_MAX_FANS)
      return false;

    FillGaps(origin, parms);
  } else if (depth < parms.set_depth) {
    for (auto &child : children)
      if (!child.FillDepth(origin, parms))
        return false; // stop searching
  }
  return true;
}

bool
FlatTriangleFanTree::FillReach(const AFlatGeoPoint &origin, const int index_low,
                               const int index_high,
                               const ReachFanParms &parms)
{
  const GeoPoint geo_origin = parms.projection.Unproject(origin);
  height = origin.altitude;

  // fill vector
  if (!IsRoot()) {
    const int index_mid = (index_high + index_low) / 2;
    const FlatGeoPoint x_mid = parms.ReachIntercept(index_mid, origin,
                                                    geo_origin);
    if (TooClose(x_mid, origin))
      return false;
  }

  AddOrigin(origin, index_high - index_low);
  for (int index = index_low; index < index_high; ++index) {
    FlatGeoPoint x = parms.ReachIntercept(index, origin, geo_origin);
    /* if ReachIntercept() did not find anything reasonable it returns
       a FlatGeoPoint that is almost the same as origin, but differs
       +/- 1 due to conversion errors. The resulting polygon can have
       overlapping edges causing triangulation failures. */
    if (AlmostTheSame(origin, x))
      x = origin;

    AddPoint(x);
  }

  return CommitPoints(IsRoot());
}

void
FlatTriangleFanTree::FillGaps(const AFlatGeoPoint &origin, ReachFanParms &parms)
{
  // worth checking for gaps?
  if (vs.size() > 2 && parms.rpolars.IsTurningReachEnabled()) {

    // now check gaps
    RouteLink e_last(RoutePoint(vs.front(), 0),
                     origin, parms.projection);
    for (auto x_last = vs.cbegin(), end = vs.cend(),
         x = x_last + 1; x != end; x_last = x++) {
      if (TooClose(*x, origin) || TooClose(*x_last, origin))
        continue;

      const RouteLink e(RoutePoint(*x, 0), origin, parms.projection);
      // check if children need to be added
      CheckGap(origin, e_last, e, parms);

      e_last = e;
    }
  }
}

void
FlatTriangleFanTree::UpdateTerrainBase(const FlatGeoPoint o,
                                       ReachFanParms &parms)
{
  if (!parms.terrain) {
    parms.terrain_base = 0;
    return;
  }

  for (const auto &x : vs) {
    const FlatGeoPoint av = (o + x) * 0.5;
    const GeoPoint p = parms.projection.Unproject(av);
    const auto h = parms.terrain->GetHeight(p);

    if (h.IsWater())
      /* water: assume 0m MSL */
      parms.terrain_counter++;
    else if (!h.IsInvalid()) {
      parms.terrain_counter++;
      parms.terrain_base += h.GetValue();
    }
  }

  if (parms.terrain_counter)
    parms.terrain_base /= parms.terrain_counter;
}

bool
FlatTriangleFanTree::CheckGap(const AFlatGeoPoint &n, const RouteLink &e_1,
                              const RouteLink &e_2, ReachFanParms &parms)
{
  const bool side = (e_1.d > e_2.d);
  const RouteLink &e_long = (side ? e_1 : e_2);
  const RouteLink &e_short = (side ? e_2 : e_1);
  if (e_short.d >= e_long.d)
    return false;

  const FlatGeoPoint &p_long = e_long.first;

  // return true if this gap was caught (applicable) whether or not it generated
  // a change

  const auto f0 = e_short.d * e_long.inv_d;
  const int h_loss =
    parms.rpolars.CalcGlideArrival(n, p_long, parms.projection) - n.altitude;

  const FlatGeoPoint dp(p_long - FlatGeoPoint(n));
  // scan from n-p_long to perpendicular to n-p_long

  int index_left, index_right;
  if (!side) {
    index_left = e_long.polar_index - REACH_SWEEP;
    index_right = e_long.polar_index - REACH_BUFFER;
  } else {
    index_left = e_long.polar_index + REACH_BUFFER;
    index_right = e_long.polar_index + REACH_SWEEP;
  }

  for (auto f = f0; f < 0.9; f += 0.1) {
    // find corner point
    const FlatGeoPoint px = (dp * f + FlatGeoPoint(n));
    // position x is length (n to p_short) along (n to p_long)
    const int h = n.altitude + f * h_loss;

    // altitude calculated from pure glide from n to x
    const AFlatGeoPoint x(px, h);

    FlatTriangleFanTree child(depth + 1);
    if (child.FillReach(x, index_left, index_right, parms)) {
      parms.vertex_counter += child.vs.size();
      parms.fan_counter++;
      children.emplace_back(std::move(child));
      return true;
    }
  }

  return false;
}

int
FlatTriangleFanTree::DirectArrival(FlatGeoPoint dest,
                                   const ReachFanParms &parms) const
{
  assert(!vs.empty());
  return parms.rpolars.CalcGlideArrival(GetOrigin(), dest, parms.projection);
}

bool
FlatTriangleFanTree::FindPositiveArrival(const FlatGeoPoint n,
                                         const ReachFanParms &parms,
                                         int &arrival_height) const
{
  if (height < arrival_height)
    return false; // can't possibly improve

  if (!bb_children.IsInside(n))
    return false; // not in scope

  if (IsInside(n)) { // found in this segment
    const int h =
      parms.rpolars.CalcGlideArrival(GetOrigin(), n, parms.projection);
    if (h > arrival_height) {
      arrival_height = h;
      return true;
    }

    /* stop here; it is impossible for a child to find a positive
       arrival height if this one didn't */
    return false;
  }

  bool retval = false;
  for (const auto &child : children)
    if (child.FindPositiveArrival(n, parms, arrival_height))
      retval = true;

  return retval;
}

void
FlatTriangleFanTree::AcceptInRange(const FlatBoundingBox &bb,
                                   FlatTriangleFanVisitor &visitor) const
{
  if (!bb.Overlaps(bb_children))
    return;

  if (bb.Overlaps(bounding_box))
    visitor.VisitFan(GetOrigin(), GetHull(IsRoot()));

  for (const auto &child : children)
    child.AcceptInRange(bb, visitor);
}
