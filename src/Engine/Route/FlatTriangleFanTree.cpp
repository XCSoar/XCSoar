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

#include "FlatTriangleFanTree.hpp"
#include "Terrain/RasterMap.hpp"
#include "ReachFanParms.hpp"
#include "Util/GlobalSliceAllocator.hpp"

#define REACH_BUFFER 1
#define REACH_SWEEP (ROUTEPOLAR_Q1-REACH_BUFFER)

// max depth set to low limit for older WINCE versions
#if !defined(_WIN32_WCE) && !defined(ANDROID)
#define REACH_MAX_DEPTH 4
#elif !defined(_WIN32_WCE) || (_WIN32_WCE >= 0x0400 && !defined(GNAV))
#define REACH_MAX_DEPTH 4
#else
#define REACH_MAX_DEPTH 3
#endif

#define REACH_MIN_STEP 25
#define REACH_MAX_VERTICES 2000

static bool
too_close(const FlatGeoPoint &p1, const FlatGeoPoint &p2)
{
  const FlatGeoPoint k = p1 - p2;
  const int dmax = std::max(abs(k.Longitude), abs(k.Latitude));
  return dmax < REACH_MIN_STEP;
}

void
FlatTriangleFanTree::CalcBB()
{
  FlatTriangleFan::calc_bb();

  bb_children = bb_self;

  for (LeafVector::iterator it = children.begin(), end = children.end();
      it != end; ++it) {
    it->CalcBB();
    bb_children.Merge(it->bb_children);
  }
}

bool
FlatTriangleFanTree::IsInsideTree(const FlatGeoPoint &p,
                                  const bool include_children) const
{
  if (include_children) {
    if (!bb_children.IsInside(p))
      return false;
  } else {
    if (!bb_self.IsInside(p))
      return false;
  }

  if (is_inside(p))
    return true;

  if (!include_children)
    return false;

  for (LeafVector::const_iterator it = children.begin(), end = children.end();
       it != end; ++it)
    if (it->IsInsideTree(p, true))
      return true;

  // should never get here!
  return false;
}

void
FlatTriangleFanTree::FillReach(const AFlatGeoPoint &origin,
                               ReachFanParms &parms)
{
  gaps_filled = false;

  FillReach(origin, 0, ROUTEPOLAR_POINTS + 1, parms);

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
  add_point(ao);
  CalcBB();
  height = ao.altitude;
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
    for (LeafVector::iterator it = children.begin(), end = children.end();
         it != end; ++it)
      if (!it->FillDepth(origin, parms))
        return false; // stop searching
  }
  return true;
}

void
FlatTriangleFanTree::FillReach(const AFlatGeoPoint &origin, const int index_low,
                               const int index_high, ReachFanParms &parms)
{
  const AGeoPoint ao(parms.task_proj.unproject(origin), origin.altitude);
  height = origin.altitude;

  // fill vector
  if (depth) {
    const int index_mid = (index_high + index_low) / 2;
    const FlatGeoPoint x_mid = parms.reach_intercept(index_mid, ao);
    if (too_close(x_mid, origin))
      return;
  }

  assert(vs.empty());
  vs.reserve(index_high - index_low + 1);
  add_point(origin);
  for (int index = index_low; index < index_high; ++index) {
    const FlatGeoPoint x = parms.reach_intercept(index, ao);
    add_point(x);
  }
}

void
FlatTriangleFanTree::FillGaps(const AFlatGeoPoint &origin, ReachFanParms &parms)
{
  // worth checking for gaps?
  if (vs.size() > 2 && parms.rpolars.IsTurningReachEnabled()) {

    // now check gaps
    const RoutePoint o(origin, RoughAltitude(0));
    RouteLink e_last(RoutePoint(*vs.begin(), RoughAltitude(0)),
                     o, parms.task_proj);
    for (VertexVector::const_iterator x_last = vs.begin(), end = vs.end(),
         x = x_last + 1; x != end; x_last = x++) {
      if (too_close(*x, origin) || too_close(*x_last, origin))
        continue;

      const RouteLink e(RoutePoint(*x, RoughAltitude(0)), o, parms.task_proj);
      // check if children need to be added
      CheckGap(origin, e_last, e, parms);

      e_last = e;
    }
  }
}

void
FlatTriangleFanTree::UpdateTerrainBase(const FlatGeoPoint &o,
                                       ReachFanParms &parms)
{
  if (!parms.terrain) {
    parms.terrain_base = 0;
    return;
  }

  for (VertexVector::const_iterator x = vs.begin(), end = vs.end();
       x != end; ++x) {
    const FlatGeoPoint av = (o + (*x)) * fixed_half;
    const GeoPoint p = parms.task_proj.unproject(av);
    short h = parms.terrain->GetHeight(p);

    if (RasterBuffer::is_water(h))
      /* water: assume 0m MSL */
      parms.terrain_counter++;
    else if (!RasterBuffer::is_invalid(h)) {
      parms.terrain_counter++;
      parms.terrain_base += h;
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

  const FlatGeoPoint &p_long = (side ? e_1.first : e_2.first);

  // return true if this gap was caught (applicable) whether or not it generated
  // a change

  const fixed f0 = e_short.d * e_long.inv_d;
  const RoughAltitude h_loss =
      parms.rpolars.CalcGlideArrival(n, p_long, parms.task_proj) - n.altitude;

  const FlatGeoPoint dp(p_long - n);
  // scan from n-p_long to perpendicular to n-p_long

  int index_left, index_right;
  if (!side) {
    index_left = e_long.polar_index - REACH_SWEEP;
    index_right = e_long.polar_index - REACH_BUFFER;
  } else {
    index_left = e_long.polar_index + REACH_BUFFER;
    index_right = e_long.polar_index + REACH_SWEEP;
  }

  children.emplace_back(depth + 1);
  FlatTriangleFanTree &child = children.back();

  for (fixed f = f0; f < fixed(0.9); f += fixed(0.1)) {
    // find corner point
    const FlatGeoPoint px = (dp * f + n);
    // position x is length (n to p_short) along (n to p_long)
    const RoughAltitude h = n.altitude + RoughAltitude(f * h_loss);

    // altitude calculated from pure glide from n to x
    const AFlatGeoPoint x(px, h);

    child.FillReach(x, index_left, index_right, parms);

    // prune child if empty or single spike
    if (child.vs.size() > 3) {
      parms.vertex_counter += child.vs.size();
      parms.fan_counter++;
      return true;
    }

    child.vs.clear();
  }

  // don't need the child
  children.pop_back();

  return false;
}

RoughAltitude
FlatTriangleFanTree::DirectArrival(const FlatGeoPoint &dest,
                                   const ReachFanParms &parms) const
{
  assert(!vs.empty());
  const AFlatGeoPoint n(vs[0], height);
  return parms.rpolars.CalcGlideArrival(n, dest, parms.task_proj);
}

bool
FlatTriangleFanTree::FindPositiveArrival(const FlatGeoPoint &n,
                                         const ReachFanParms &parms,
                                         RoughAltitude &arrival_height) const
{
  if (height < arrival_height)
    return false; // can't possibly improve

  if (!bb_children.IsInside(n))
    return false; // not in scope

  if (is_inside(n)) { // found in this segment
    const AFlatGeoPoint nn(vs[0], height);
    const RoughAltitude h =
      parms.rpolars.CalcGlideArrival(nn, n, parms.task_proj);
    if (h > arrival_height) {
      arrival_height = h;
      return true;
    }
  }

  bool retval = false;
  for (LeafVector::const_iterator it = children.begin(), end = children.end();
      it != end; ++it)
    if (it->FindPositiveArrival(n, parms, arrival_height))
      retval = true;

  return retval;
}

void
FlatTriangleFanTree::AcceptInRange(const FlatBoundingBox &bb,
                                   const TaskProjection &task_proj,
                                   TriangleFanVisitor &visitor) const
{
  if (!bb.Overlaps(bb_children))
    return;

  if (bb.Overlaps(bb_self)) {
    visitor.StartFan();
    for (VertexVector::const_iterator it = vs.begin(), end = vs.end();
         it != end; ++it)
      visitor.AddPoint(task_proj.unproject(*it));

    visitor.EndFan();
  }

  for (LeafVector::const_iterator it = children.begin(), end = children.end();
       it != end; ++it)
    it->AcceptInRange(bb, task_proj, visitor);
}
