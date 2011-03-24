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
#include "Geo/GeoBounds.hpp"

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

struct ReachFanParms {
  ReachFanParms(const RoutePolars& _rpolars,
                const TaskProjection& _task_proj,
                const short _terrain_base,
                const RasterMap* _terrain=NULL):
    rpolars(_rpolars), task_proj(_task_proj), terrain(_terrain), 
    terrain_base(_terrain_base),
    terrain_counter(0),
    fan_counter(0) {};

  const RoutePolars &rpolars;
  const TaskProjection& task_proj;
  const RasterMap* terrain;
  int terrain_base;
  int terrain_counter;
  int fan_counter;

  FlatGeoPoint reach_intercept(const int index,
                               const AGeoPoint& ao) const {
    return rpolars.reach_intercept(index, ao, terrain, task_proj);
  }
};

static bool too_close(const FlatGeoPoint& p1, const FlatGeoPoint& p2)
{
  const FlatGeoPoint k = p1-p2;
  const int dmax = std::max(abs(k.Longitude), abs(k.Latitude));
  return dmax < REACH_MIN_STEP;
}

void FlatTriangleFan::calc_bb() {
  bb_self = FlatBoundingBox(vs[0]);
  for (std::vector<FlatGeoPoint>::const_iterator it = vs.begin();
       it != vs.end(); ++it) {
    bb_self.expand(*it);
  }
}

void
FlatTriangleFanTree::calc_bb() {
  FlatTriangleFan::calc_bb();

  bb_children = bb_self;

  for (std::vector<FlatTriangleFanTree>::const_iterator it = children.begin();
       it != children.end(); ++it) {
    bb_children.expand(it->bb_children);
  }
};

//////////

void
FlatTriangleFan::add_point(const FlatGeoPoint &p) {
  if (!vs.empty()) {
    if (p == vs.back())
      return; // dont add duplicates
  }
  vs.push_back(p);
}

//////////

bool
FlatTriangleFan::is_inside(const FlatGeoPoint &p) const {
  if (!bb_self.is_inside(p))
    return false;

  int c=0;
  for (std::vector<FlatGeoPoint>::const_iterator i= vs.begin(), j=vs.end()-1;
       i!= vs.end(); j = i++) {
    if ((i->Latitude>p.Latitude) == (j->Latitude>p.Latitude))
      continue;
    if (( p.Longitude < (j->Longitude-i->Longitude)*(p.Latitude-i->Latitude)/(j->Latitude-i->Latitude)
          + i->Longitude))
      c = !c;
  }
  return (c>0);
};

bool
FlatTriangleFanTree::is_inside_tree(const FlatGeoPoint &p, const bool include_children) const {
  if (include_children) {
    if (!bb_children.is_inside(p))
      return false;
  } else {
    if (!bb_self.is_inside(p))
      return false;
  }
  //
  if (is_inside(p))
    return true;

  if (!include_children)
    return false;

  for (std::vector<FlatTriangleFanTree>::const_iterator it = children.begin();
       it != children.end(); ++it) {
    if (it->is_inside_tree(p, true))
      return true;
  }
  // should never get here!
  return false;
};

/////////////

void
FlatTriangleFanTree::fill_reach(const AFlatGeoPoint &origin,
                                ReachFanParms& parms) {
  height = origin.altitude;
  fill_reach(origin, 0, ROUTEPOLAR_POINTS+1, parms);
}

void
FlatTriangleFanTree::fill_reach(const AFlatGeoPoint &origin,
                                const int index_low, const int index_high,
                                ReachFanParms& parms) {
  const AGeoPoint ao (parms.task_proj.unproject(origin), origin.altitude);

  // fill vector
  if (depth>0) {
    const int index_mid = (index_high+index_low)/2;
    const FlatGeoPoint x_mid = parms.reach_intercept(index_mid, ao);
    if (too_close(x_mid, origin))
      return;
  }
  add_point(origin);
  for (int index= index_low; index< index_high; ++index) {
    const FlatGeoPoint x = parms.reach_intercept(index, ao);
    add_point(x);
  }

  // worth checking for gaps?
  if ((depth< REACH_MAX_DEPTH) && (vs.size()>2) && (parms.rpolars.turning_reach())) {
    // now check gaps
    const RoutePoint o(origin, 0);
    RouteLink e_last(RoutePoint(*vs.begin(), 0), o, parms.task_proj);
    for (std::vector<FlatGeoPoint>::const_iterator x_last= vs.begin(), x= x_last+1;
         x != vs.end(); x_last = x++) {
      if (too_close(*x, origin) || too_close(*x_last, origin))
        continue;

      const RouteLink e(RoutePoint(*x, 0), o, parms.task_proj);
      // check if children need to be added
      check_gap(origin, e_last, e, parms);

      e_last = e;
    }
  }

  // update bounding box
  calc_bb();
}

void
FlatTriangleFanTree::update_terrain_base(const FlatGeoPoint& o, ReachFanParms& parms)
{
  if (!parms.terrain) {
    parms.terrain_base = 0;
    return;
  }
  for (std::vector<FlatGeoPoint>::const_iterator x= vs.begin(); 
       x != vs.end(); ++x) {
    const FlatGeoPoint av = (o+(*x))*fixed_half;
    const GeoPoint p = parms.task_proj.unproject(av);
    short h = parms.terrain->GetHeight(p);
    if (!RasterBuffer::is_invalid(h)) {
      parms.terrain_counter++;
      parms.terrain_base+= h;
    }
  }
  if (parms.terrain_counter)
    parms.terrain_base/= parms.terrain_counter;
}


bool
FlatTriangleFanTree::check_gap(const AFlatGeoPoint& n,
                               const RouteLink& e_1,
                               const RouteLink& e_2,
                               ReachFanParms& parms)
{
  const bool side = (e_1.d > e_2.d);
  const RouteLink& e_long = (side)? e_1: e_2;
  const RouteLink& e_short = (side)? e_2: e_1;
  if (e_short.d >= e_long.d)
    return false;

  const FlatGeoPoint& p_long = (side)? e_1.first: e_2.first;

  // return true if this gap was caught (applicable) whether or not it generated
  // a change

  const fixed f0 = e_short.d/e_long.d;
  const short h_loss = parms.rpolars.calc_glide_arrival(n, p_long, parms.task_proj)
    -n.altitude;

  const FlatGeoPoint dp(p_long-n);
  // scan from n-p_long to perpendicular to n-p_long
  const int index = RoutePolar::dxdy_to_index(dp.Longitude, dp.Latitude);

  int index_left, index_right;
  if (!side) {
    index_left = index-REACH_SWEEP;
    index_right = index-REACH_BUFFER;
  } else {
    index_left = index+REACH_BUFFER;
    index_right = index+REACH_SWEEP;
  }

  for (fixed f= f0; f< fixed(0.9); f+= fixed(0.1)) {
    // find corner point
    const FlatGeoPoint px = (dp*f+n);
    // position x is length (n to p_short) along (n to p_long)
    const short h = n.altitude+(short)(f*h_loss);

    // altitude calculated from pure glide from n to x
    const AFlatGeoPoint x(px, h);

    children.push_back(FlatTriangleFanTree(depth+1));
    std::vector<FlatTriangleFanTree>::reverse_iterator it = children.rbegin();
    it->height = h;
    it->fill_reach(x, index_left, index_right, parms);

    // prune child if empty or single spike
    if (it->vs.size()>3) {
      parms.fan_counter++;
      return true;
    }
    children.pop_back();
  }
  return false;
}


void ReachFan::reset() {
  AbstractReach::reset();
  root.clear();
  terrain_base = 0;
}

bool ReachFan::solve(const AGeoPoint origin,
                     const RoutePolars &rpolars,
                     const RasterMap* terrain) {
  reset();

  if (!AbstractReach::solve(origin, rpolars, terrain))
    return false;

  const short h = terrain? terrain->GetHeight(origin): 0;

  fan_size = 0;

  if (terrain && (origin.altitude <= h + rpolars.safety_height()))
    return false;

  ReachFanParms parms(rpolars, task_proj, terrain_base, terrain);
  const AFlatGeoPoint ao(task_proj.project(origin), origin.altitude);

  root.fill_reach(ao, parms);
  fan_size = parms.fan_counter;

  if (!RasterBuffer::is_invalid(h)) {
    parms.terrain_base = h;
    parms.terrain_counter = 1;
  } else {
    parms.terrain_base = 0;
    parms.terrain_counter = 0;
  }
  if (parms.terrain) {
    root.update_terrain_base(ao, parms);
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
  return root.is_inside_tree(p, turning);
}


bool
ReachFan::find_positive_arrival(const AGeoPoint dest,
                                const RoutePolars &rpolars,
                                short& arrival_height) const
{
  const FlatGeoPoint d (task_proj.project(dest));
  if (dest.altitude>= root.get_height()) {
    arrival_height = -1;
    return true;
  }
  arrival_height = dest.altitude-1;
  ReachFanParms parms(rpolars, task_proj, terrain_base);
  root.find_positive_arrival(d, parms, arrival_height);
  if (arrival_height < dest.altitude)
    arrival_height = -1;
  return true;
}

bool
FlatTriangleFanTree::find_positive_arrival(const FlatGeoPoint& n,
                                           const ReachFanParms& parms,
                                           short& arrival_height) const
{
  if (height < arrival_height)
    return false; // can't possibly improve

  if (!bb_children.is_inside(n))
    return false; // not in scope

  if (is_inside(n)) { // found in this segment
    const AFlatGeoPoint nn (vs[0], height);
    const short h = parms.rpolars.calc_glide_arrival(nn, n, parms.task_proj);
    if (h > arrival_height) {
      arrival_height = h;
      return true;
    }
  }

  bool retval = false;

  for (std::vector<FlatTriangleFanTree>::const_iterator it = children.begin();
       it != children.end(); ++it) {
    if (it->find_positive_arrival(n, parms, arrival_height))
      retval = true;
  }
  return retval;
}

void
FlatTriangleFanTree::accept_in_range(const FlatBoundingBox& bb,
                                     const TaskProjection& task_proj,
                                     TriangleFanVisitor& visitor) const
{
  if (!bb.overlaps(bb_children))
    return;

  if (bb.overlaps(bb_self)) {
    visitor.start_fan();
    for (std::vector<FlatGeoPoint>::const_iterator it = vs.begin();
         it != vs.end(); ++it) {
      visitor.add_point(task_proj.unproject(*it));
    }
    visitor.end_fan();
  }
  for (std::vector<FlatTriangleFanTree>::const_iterator it = children.begin();
       it != children.end(); ++it) {
    it->accept_in_range(bb, task_proj, visitor);
  }
}

void
ReachFan::accept_in_range(const GeoBounds& bounds,
                          TriangleFanVisitor& visitor) const
{
  const FlatBoundingBox bb = task_proj.project(bounds);
  visitor.allocate_fans(fan_size);
  root.accept_in_range(bb, task_proj, visitor);
}
