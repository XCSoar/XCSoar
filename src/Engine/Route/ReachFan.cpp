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
#define REACH_MAX_DEPTH 4

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

  const int nvert = vs.size();

  int i, j, c = 0;
  for (i = 0, j = nvert-1; i < nvert; j = i++) {
    if ((vs[i].Latitude>p.Latitude) == (vs[j].Latitude>p.Latitude))
      continue;
    if (( p.Longitude < (vs[j].Longitude-vs[i].Longitude)*(p.Latitude-vs[i].Latitude)/(vs[j].Latitude-vs[i].Latitude)
          + vs[i].Longitude))
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
                                const RoutePolars &rpolars,
                                const RasterMap* terrain,
                                const TaskProjection& task_proj) {
  height = origin.altitude;
  fill_reach(origin, 0, ROUTEPOLAR_POINTS+1, rpolars, terrain, task_proj);
}

void
FlatTriangleFanTree::fill_reach(const AFlatGeoPoint &origin,
                                const int index_low, const int index_high,
                                const RoutePolars &rpolars,
                                const RasterMap* terrain,
                                const TaskProjection& task_proj) {
  add_point(origin);

  const AGeoPoint ao (task_proj.unproject(origin), origin.altitude);

  for (int index= index_low; index< index_high; ++index) {
    const FlatGeoPoint x = rpolars.reach_intercept(index, ao, terrain, task_proj);
    const FlatGeoPoint x_last = vs.back();

    if (x== x_last)
      continue;

    add_point(x);

    if (depth>= REACH_MAX_DEPTH)
      continue;
    if (index== index_low)
      continue;

    // check if children need to be added
    check_gap(origin, x_last, x, rpolars, terrain, task_proj, -1)
      || check_gap(origin, x, x_last, rpolars, terrain, task_proj, 1);

  }
  calc_bb();
}

bool
FlatTriangleFanTree::check_gap(const AFlatGeoPoint& n,
                               const FlatGeoPoint& p_long,
                               const FlatGeoPoint& p_short,
                               const RoutePolars &rpolars,
                               const RasterMap* terrain,
                               const TaskProjection& task_proj,
                               const int side)
{
  const RouteLink e_long(RoutePoint(p_long, 0), RoutePoint(n, 0), task_proj);
  const RouteLink e_short(RoutePoint(p_short, 0), RoutePoint(n, 0), task_proj);

  // return true if this gap was caught (applicable) whether or not it generated
  // a change

  if (e_short.d < fixed(1000.0))
    return false;
  if (e_short.d >= e_long.d)
    return false;

  const fixed f = e_short.d/e_long.d;
  if (f> fixed(0.9))
    return false;

  // find corner point
  const FlatGeoPoint px = // ((p_long-n)*f+n);
    p_long*f+n*(fixed_one-f);

  // position x is length (n to p_short) along (n to p_long)

  const short h = rpolars.calc_glide_arrival(n, px, task_proj);

  AFlatGeoPoint x(px, h);
  // altitude calculated from pure glide from n to x

  const RouteLink e_new(RoutePoint(p_long, 0), x, task_proj);

  // see if there is sufficient height available to be worth extending the search
  const short terrain_height_x = terrain->GetField(task_proj.unproject(px));
  const short h_excess = x.altitude -
    (terrain_height_x + rpolars.safety_height());
  if (h_excess <= 0)
    return true;

  // scan from n-p_long to perpendicular to n-p_long

  int index_left, index_right;

  if (side==1) {
    index_left = (int)e_new.polar_index-REACH_SWEEP;
    index_right = (int)e_new.polar_index-REACH_BUFFER;
  } else if (side==-1) {
    index_left = e_new.polar_index+REACH_BUFFER;
    index_right = e_new.polar_index+REACH_SWEEP;
  } else {
    assert(1);
  }

  children.push_back(FlatTriangleFanTree(depth+1));
  std::vector<FlatTriangleFanTree>::reverse_iterator it = children.rbegin();
  it->height = h;
  it->fill_reach(x, index_left, index_right, rpolars, terrain, task_proj);

  // prune child if empty or single spike
  if (it->vs.size()<4) {
    children.pop_back();
  }
  return true;
}


void ReachFan::reset() {
  AbstractReach::reset();
  root.clear();
}

bool ReachFan::solve(const AGeoPoint origin,
                   const RoutePolars &rpolars,
                   const RasterMap* terrain) {
  reset();

  if (!AbstractReach::solve(origin, rpolars, terrain))
    return false;

  const AFlatGeoPoint ao(task_proj.project(origin), origin.altitude);
  root.fill_reach(ao, rpolars, terrain, task_proj);
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
  root.find_positive_arrival(d, rpolars, task_proj, arrival_height);
  if (arrival_height < dest.altitude)
    arrival_height = -1;
  return true;
}

bool
FlatTriangleFanTree::find_positive_arrival(const FlatGeoPoint& n,
                                           const RoutePolars &rpolars,
                                           const TaskProjection& task_proj,
                                           short& arrival_height) const
{
  if (height < arrival_height)
    return false; // can't possibly improve

  if (!bb_children.is_inside(n))
    return false; // not in scope

  if (is_inside(n)) { // found in this segment
    const AFlatGeoPoint nn (vs[0], height);
    const short h = rpolars.calc_glide_arrival(nn, n, task_proj);
    if (h > arrival_height) {
      arrival_height = h;
      return true;
    }
  }

  bool retval = false;

  for (std::vector<FlatTriangleFanTree>::const_iterator it = children.begin();
       it != children.end(); ++it) {
    if (it->find_positive_arrival(n, rpolars, task_proj, arrival_height))
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
  root.accept_in_range(bb, task_proj, visitor);
}
