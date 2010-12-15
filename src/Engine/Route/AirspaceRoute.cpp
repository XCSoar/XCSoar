/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "AirspaceRoute.hpp"
#include "Navigation/SearchPointVector.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Navigation/ConvexHull/PolygonInterior.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Math/FastMath.h"
#include <limits.h>

using std::make_pair;

/////////// Airspace query helpers

/**
 * Find airspace and location of nearest intercept
 */
class AIV: public AirspaceIntersectionVisitor
{
public:
  typedef std::pair<const AbstractAirspace*, GeoPoint> AIVResult;

  AIV(const GeoPoint& _origin):
    AirspaceIntersectionVisitor(),
    origin(_origin),
    min_distance(-fixed_one),
    nearest(NULL,_origin)
    {
    }
  virtual void Visit(const AirspaceCircle &as) {
    visit_abstract(as);
  }
  virtual void Visit(const AirspacePolygon &as) {
    visit_abstract(as);
  }
  void visit_abstract(const AbstractAirspace &as) {
    assert(!m_intersections.empty());

    fixed d;
    GeoPoint point = m_intersections[0].first;
    d= point.distance(origin);
    if (negative(min_distance) || (d < min_distance)) {
      min_distance = d;
      nearest = std::make_pair(&as, point);
    }
    if (m_intersections.size()>1) {
      point = m_intersections[1].first;
      fixed dd= point.distance(origin);
      assert (dd>=d);
      if (dd < min_distance) {
        min_distance = dd;
        nearest = std::make_pair(&as, point);
      }
    }
  }

  AIVResult get_nearest() const
    {
      return nearest;
    }
private:
  const GeoPoint& origin;
  fixed min_distance;
  AIVResult nearest;
};


class AirspaceInsideOtherVisitor: public AirspaceVisitor {
public:
  AirspaceInsideOtherVisitor():
    m_found(NULL) {};

  const AbstractAirspace* found() {
    return m_found;
  }

protected:
  virtual void Visit(const AirspaceCircle &as) {
    visit_abstract(as);
  }
  virtual void Visit(const AirspacePolygon &as) {
    visit_abstract(as);
  }
  virtual void visit_abstract(const AbstractAirspace &as) {
    m_found = &as;
  }
private:
  const AbstractAirspace* m_found;
};


AirspaceRoute::RouteIntersection
AirspaceRoute::first_intersecting(const GeoPoint& origin,
                                  const GeoPoint& dest) const
{
  const GeoVector v(origin, dest);
  AIV visitor(origin);
  m_airspaces.visit_intersecting(origin, v, visitor);
  const AIV::AIVResult res (visitor.get_nearest());
  count_airspace++;
  return std::make_pair(res.first,
                        SearchPoint(res.second,
                                    m_airspaces.get_task_projection()).get_flatLocation());
}

AirspaceRoute::RouteIntersection
AirspaceRoute::first_intersecting(const RouteLink& e) const
{
  const GeoPoint g1 = m_airspaces.get_task_projection().unproject(e.first);
  const GeoPoint g2 = m_airspaces.get_task_projection().unproject(e.second);
  return first_intersecting(g1, g2);
}

const AbstractAirspace*
AirspaceRoute::inside_others(const GeoPoint& origin) const
{
  AirspaceInsideOtherVisitor visitor;
  m_airspaces.visit_within_range(origin, fixed_one, visitor);
  count_airspace++;
  return visitor.found();
}


////////// Node generation utilities

AirspaceRoute::ClearingPair
AirspaceRoute::find_clearing_pair(const SearchPointVector& spv,
                                  const SearchPointVector::const_iterator start,
                                  const SearchPointVector::const_iterator end,
                                  const RoutePoint dest) const
{
  bool backwards = false;
  ClearingPair p(dest, dest);

  bool check_others = false;

  bool found_left = false;
  bool found_right = false;

  SearchPointVector::const_iterator i= start;

  int j=0;
  while ((i != end)&&(j<2)) {
    FlatGeoPoint pborder = i->get_flatLocation();
    const FlatRay ray(pborder, dest);

    if (intersects(spv, ray)) {
      j++;
      if (j==1) {
        i = start;
        backwards = true;
        continue;
      }
    } else {
      GeoPoint gborder = m_airspaces.get_task_projection().unproject(pborder);
      if (!check_others || !inside_others(gborder)) {
        if (j==0) {
          found_left = true;
          p.first = pborder;
        } else if (j==1) {
          found_right = true;
          p.second = pborder;
        }
      }
    }

    if (backwards)
      circular_previous(i, spv);
    else
      circular_next(i, spv);
  }
  return p;
}

AirspaceRoute::ClearingPair
AirspaceRoute::get_pairs(const SearchPointVector& spv,
                         const RoutePoint start,
                         const RoutePoint dest) const
{
  SearchPointVector::const_iterator i_closest =
    nearest_index_convex(spv, start);
  SearchPointVector::const_iterator i_furthest =
    nearest_index_convex(spv, dest);
  ClearingPair p = find_clearing_pair(spv, i_closest, i_furthest, start);
  return p;
}

AirspaceRoute::ClearingPair
AirspaceRoute::get_backup_pairs(const SearchPointVector& spv,
                         const RoutePoint intc) const
{
  SearchPointVector::const_iterator start = nearest_index_convex(spv, intc);
  ClearingPair p(intc, intc);

  SearchPointVector::const_iterator i_left = start;
  circular_next(i_left, spv);
  p.first = i_left->get_flatLocation();

  SearchPointVector::const_iterator i_right = start;
  circular_previous(i_right, spv);
  p.second = i_right->get_flatLocation();

  return p;
}


////////////////

unsigned
AirspaceRoute::airspace_size() const
{
  return m_airspaces.size();
}

AirspaceRoute::AirspaceRoute(const Airspaces& master):
  m_airspaces(master, false),
  m_planner()
{
  reset();
}

AirspaceRoute::~AirspaceRoute()
{
  // clean up, we dont need the clearances any more
  m_airspaces.clear_clearances();
}

void
AirspaceRoute::reset()
{
  origin_last = FlatGeoPoint(0,0);
  destination_last = FlatGeoPoint(0,0);
  dirty = false;
  m_airspaces.clear_clearances();
  m_airspaces.clear();
  solution_route.clear();
  m_planner.clear();
  m_unique.clear();
}

void
AirspaceRoute::get_solution(Route& route) const
{
  route = solution_route;
}

void
AirspaceRoute::synchronise(const Airspaces& master,
                           const GeoPoint& origin,
                           const GeoPoint& destination,
                           const AirspacePredicate &condition)
{
  // @todo: also synchronise with AirspaceWarningManager to filter out items that are
  // acknowledged.
  GeoVector vector(origin, destination);
  if (m_airspaces.synchronise_in_range(master, vector.mid_point(origin), vector.Distance/2, condition))
  {
    if (m_airspaces.size())
      dirty = true;
  }
}

bool
AirspaceRoute::solve(const GeoPoint& origin,
                     const GeoPoint& destination)
{
  const SearchPoint s_origin(origin, m_airspaces.get_task_projection());
  const SearchPoint s_destination(destination, m_airspaces.get_task_projection());

  if ((s_origin.get_flatLocation() == origin_last)
      && (s_destination.get_flatLocation() == destination_last)
      && !dirty) {
    return false; // solution was not updated
  }
  if (!m_airspaces.size()) {
    return false; // trivial solution
  }

  dirty = false;
  origin_last = s_origin.get_flatLocation();
  destination_last = s_destination.get_flatLocation();

  solution_route.clear();
  solution_route.push_back(origin);
  solution_route.push_back(destination);

  RoutePoint start(s_origin.get_flatLocation());
  m_astar_goal = RoutePoint(s_destination.get_flatLocation());
  RouteLink estart(start, m_astar_goal);

  if (distance(estart)<= ROUTE_MIN_STEP)
    return false;

  count_dij=0;
  count_airspace=0;

  bool retval = false;
  m_planner.restart(start);

  unsigned best_d = UINT_MAX;

  while (!m_planner.empty()) {
    const RoutePoint node = m_planner.pop();

    if (node == m_astar_goal)
      retval = true;

    { // copy improving solutions
      Route this_solution;
      unsigned d = find_solution(node, this_solution);
      if ((d< best_d) || retval) {
        best_d = d;
        solution_route = this_solution;
      }
    }
    if (retval)
      break; // want top solution only

    // shoot for final
    add_candidate(RouteLink(node, m_astar_goal));
    while (!m_links.empty()) {
      add_edges(m_links.front());
      m_links.pop();
    }
  }
  count_unique = m_unique.size();

  if (retval) {
    // correct for rounding
    if (solution_route.size()>0)
      solution_route[0] = origin;
    if (solution_route.size()>1)
      solution_route[solution_route.size()-1] = destination;
  }

  m_planner.clear();
  m_unique.clear();
  return retval;
}


unsigned
AirspaceRoute::find_solution(const RoutePoint &final, Route& this_route) const
{
  RoutePoint p(final);
  RoutePoint p_last(p);
  do {
    p_last = p;
    p = m_planner.get_predecessor(p);
    this_route.insert(this_route.begin(),
                      m_airspaces.get_task_projection().unproject(p_last));
  } while (!(p == p_last));

  return distance(RouteLink(final, m_astar_goal));
}


////////////////

void
AirspaceRoute::link_cleared(const RouteLink &e)
{
  count_dij++;
  AStarPriorityValue v(distance(e),
                       distance(RouteLink(e.second, m_astar_goal)));
  m_planner.link(e.second, e.first, v);
}

void
AirspaceRoute::add_candidate(const RouteLink e)
{
  if (!is_short(e)) {
    if (m_unique.find(e) == m_unique.end()) {
      m_links.push(e);
      m_unique.insert(e);
    }
  }
}

void
AirspaceRoute::add_nearby(const RouteIntersection &inx,
                          const RouteLink &e)
{
  const SearchPointVector& fat = inx.first->get_clearance();
  ClearingPair p = get_pairs(fat, e.first, e.second);
  ClearingPair pb = get_backup_pairs(fat, inx.second);

  // process both options
  add_candidate(RouteLink(e.first, p.first));
  add_candidate(RouteLink(e.first, p.second));
  add_candidate(RouteLink(e.first, pb.first));
  add_candidate(RouteLink(e.first, pb.second));
}


void
AirspaceRoute::add_edges(const RouteLink &e)
{
  const RouteIntersection inx = first_intersecting(e);
  const bool this_short = is_short(e);

  if (inx.first==NULL) // does not intersect
  {
    if (!this_short)
      link_cleared(e);

    add_candidate(RouteLink(m_planner.get_predecessor(e.first), e.second));
    return;
  }

  if (!this_short)
    add_nearby(inx, e);
}


/*
consider pa: trying for pb

  for first intersection found, obstacle A,

    follow border both ways** to find point p1 on A such that p1-pb wont intersect with A.

    if pa-p1 also doesnt intersect with A,
       if can add one stage
         then pa->p1 is linked and p1 is added as candidate*
    else
       if can add two stages

         then follow border both ways** to find p2 (distance p2-b smaller than p1-b),
         that does not intersect with A.

         now pa->p1 and p1->p2 are linked and p1 and p2 are added as candidates*

         actually this is for entire paths

  if no intersections, link pa->pb

  mark a as done (pop)

* dont add if already exists
** follow border up to nearest point index towards pb;
     this may be an intermediate point if we descend

  // candidates may collapse if can clear both directions, if they
  // dont we need all points between the to-start, to-end calls

------------

  Allow entry into airspace if destination point is final and it is in that airspace
  Ditto with start

  Maintain airspace cache:
  - keep persistent
  - dont copy basic airspace, merge items and convert to convex hull fattened
  - merge test: check bounding box overlap first, then

  - visit master regularly with task info,
       - check if items to be added or deleted
       - delete if entirely inside does nothing
       - add new if entirely outside

  - this means we *do* own the airspaces

*/
