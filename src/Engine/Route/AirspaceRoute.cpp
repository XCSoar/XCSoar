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
#include "Navigation/Geometry/GeoVector.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/Predicate/AirspacePredicateHeightRange.hpp"
#include "Math/FastMath.h"

// Airspace query helpers

/**
 * Find airspace and location of nearest intercept
 */
class AIV: public AirspaceIntersectionVisitor
{
public:
  typedef std::pair<const AbstractAirspace*, RoutePoint> AIVResult;

  AIV(const RouteLink& _e,
      const TaskProjection& _proj,
      const RoutePolars& _rpolar):
    AirspaceIntersectionVisitor(),
    link(_e),
    min_distance(-fixed_one),
    proj(_proj),
    rpolar(_rpolar),
    origin(proj.unproject(_e.first)),
    nearest((const AbstractAirspace *)NULL, _e.first)
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

    GeoPoint point = m_intersections[0].first;

    RouteLink l = rpolar.generate_intermediate(link.first,
                                               RoutePoint(proj.project(point), link.second.altitude),
                                               proj);

    if ((l.second.altitude< (short)as.GetBase().altitude) ||
        (l.second.altitude> (short)as.GetTop().altitude))
      return;

    if (negative(min_distance) || (l.d < min_distance)) {
      min_distance = l.d;
      nearest = std::make_pair(&as, l.second);
    }
  }

  AIVResult get_nearest() const {
    return nearest;
  }
private:
  const RouteLink& link;
  fixed min_distance;
  const TaskProjection& proj;
  const RoutePolars& rpolar;
  const GeoPoint& origin;
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


AirspaceRoute::RouteAirspaceIntersection
AirspaceRoute::first_intersecting(const RouteLink& e) const
{
  const GeoPoint origin(task_projection.unproject(e.first));
  const GeoPoint dest(task_projection.unproject(e.second));
  const GeoVector v(origin, dest);
  AIV visitor(e, task_projection, rpolars_route);
  m_airspaces.visit_intersecting(origin, v, visitor);
  const AIV::AIVResult res (visitor.get_nearest());
  count_airspace++;
  return std::make_pair(res.first, res.second);
}

const AbstractAirspace*
AirspaceRoute::inside_others(const AGeoPoint& origin) const
{
  AirspaceInsideOtherVisitor visitor;
  m_airspaces.visit_within_range(origin, fixed_one, visitor);
  count_airspace++;
  return visitor.found();
}


// Node generation utilities

AirspaceRoute::ClearingPair
AirspaceRoute::find_clearing_pair(const SearchPointVector& spv,
                                  const SearchPointVector::const_iterator start,
                                  const SearchPointVector::const_iterator end,
                                  const RoutePoint &dest) const
{
  bool backwards = false;
  ClearingPair p(dest, dest);

  bool check_others = false;

  SearchPointVector::const_iterator i= start;

  int j=0;
  while ((i != end)&&(j<2)) {
    AFlatGeoPoint pborder(i->get_flatLocation(), dest.altitude); // @todo alt!
    const FlatRay ray(pborder, dest);

    if (spv.IntersectsWith(ray)) {
      j++;
      if (j==1) {
        i = start;
        backwards = true;
        continue;
      }
    } else {
      AGeoPoint gborder(task_projection.unproject(pborder), dest.altitude); // @todo alt!
      if (!check_others || !inside_others(gborder)) {
        if (j==0) {
          p.first = pborder;
        } else if (j==1) {
          p.second = pborder;
        }
      }
    }

    if (backwards)
      spv.PreviousCircular(i);
    else
      spv.NextCircular(i);
  }
  return p;
}

AirspaceRoute::ClearingPair
AirspaceRoute::get_pairs(const SearchPointVector& spv,
                         const RoutePoint &start,
                         const RoutePoint &dest) const
{
  SearchPointVector::const_iterator i_closest =
    spv.NearestIndexConvex(start);
  SearchPointVector::const_iterator i_furthest =
    spv.NearestIndexConvex(dest);
  ClearingPair p = find_clearing_pair(spv, i_closest, i_furthest, start);
  return p;
}

AirspaceRoute::ClearingPair
AirspaceRoute::get_backup_pairs(const SearchPointVector& spv,
                                const RoutePoint &_start,
                                const RoutePoint &intc) const
{
  SearchPointVector::const_iterator start = spv.NearestIndexConvex(intc);
  ClearingPair p(intc, intc);

  SearchPointVector::const_iterator i_left = start;
  spv.NextCircular(i_left);
  p.first = AFlatGeoPoint(i_left->get_flatLocation(), _start.altitude); // @todo alt!

  SearchPointVector::const_iterator i_right = start;
  spv.PreviousCircular(i_right);
  p.second = AFlatGeoPoint(i_right->get_flatLocation(), _start.altitude); // @todo alt!

  return p;
}


////////////////

unsigned
AirspaceRoute::airspace_size() const
{
  return m_airspaces.size();
}

AirspaceRoute::AirspaceRoute(const Airspaces& master):
  m_airspaces(master, false)
{
  Reset();
}

AirspaceRoute::~AirspaceRoute()
{
  // clean up, we dont need the clearances any more
  m_airspaces.clear_clearances();
}

void
AirspaceRoute::Reset()
{
  RoutePlanner::Reset();
  m_airspaces.clear_clearances();
  m_airspaces.clear();
}

void
AirspaceRoute::synchronise(const Airspaces& master,
                           const AGeoPoint& origin,
                           const AGeoPoint& destination)
{
  // @todo: also synchronise with AirspaceWarningManager to filter out items that are
  // acknowledged.
  GeoVector vector(origin, destination);
  h_min = std::min(origin.altitude, std::min(destination.altitude, h_min));
  h_max = std::max(origin.altitude, std::max(destination.altitude, h_max));
  // @todo: have margin for h_max to allow for climb
  AirspacePredicateHeightRangeExcludeTwo condition(h_min, h_max, origin, destination);
  if (m_airspaces.synchronise_in_range(master, vector.mid_point(origin), vector.Distance/2, condition))
  {
    if (m_airspaces.size())
      dirty = true;
  }
}

void
AirspaceRoute::add_nearby_airspace(const RouteAirspaceIntersection &inx,
                                   const RouteLink &e)
{
  const SearchPointVector& fat = inx.first->GetClearance();
  const ClearingPair p = get_pairs(fat, e.first, e.second);
  const ClearingPair pb = get_backup_pairs(fat, e.first, inx.second);

  // process all options
  AddCandidate(RouteLinkBase(e.first, p.first));
  AddCandidate(RouteLinkBase(e.first, p.second));
  AddCandidate(RouteLinkBase(e.first, pb.first));
  AddCandidate(RouteLinkBase(e.first, pb.second));
}

void
AirspaceRoute::AddNearby(const RouteLink &e)
{
  RoutePoint ptmp = m_inx.second;
  if (m_inx.first == NULL)
    AddNearbyTerrain(ptmp, e);
  else
    add_nearby_airspace(m_inx, e);
}

bool
AirspaceRoute::CheckSecondary(const RouteLink &e)
{
  if (!rpolars_route.airspace_enabled())
    return true; // trivial

  m_inx = first_intersecting(e);
  if (m_inx.first!=NULL)  {
    AddCandidate(e);
    return false;
  };
  return true;
}


bool
AirspaceRoute::CheckClearance(const RouteLink &e, RoutePoint& inp) const
{
  // attempt terrain clearance first

  if (!CheckClearanceTerrain(e, inp)) {
    m_inx.first = NULL;
    m_inx.second = inp;
    return false;
  }

  if (!rpolars_route.airspace_enabled())
    return true; // trivial

  // passes terrain, so now check airspace clearance

  m_inx = first_intersecting(e);
  if (m_inx.first!=NULL)  {
    inp = m_inx.second;
    return false;
  }

  // made it this far!
  return true;
}


void
AirspaceRoute::OnSolve(const AGeoPoint& origin,
                        const AGeoPoint& destination)
{
  if (m_airspaces.empty()) {
    task_projection.reset(origin);
    task_projection.update_fast();
  } else {
    task_projection = m_airspaces.get_task_projection();
  }
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

