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

#include "AirspaceRoute.hpp"
#include "Geo/SearchPointVector.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/Predicate/AirspacePredicateHeightRange.hpp"
#include "Airspace/Predicate/AirspacePredicate.hpp"
#include "Geo/Flat/FlatRay.hpp"

// Airspace query helpers

/**
 * Find airspace and location of nearest intercept
 */
class AIV final : public AirspaceIntersectionVisitor {
public:
  typedef std::pair<const AbstractAirspace *, RoutePoint> AIVResult;

private:
  const RouteLink &link;
  double min_distance;
  const FlatProjection &proj;
  const RoutePolars &rpolar;
  AIVResult nearest;

public:
  AIV(const RouteLink &_e,
      const FlatProjection &_proj,
      const RoutePolars &_rpolar)
   :link(_e),
    min_distance(-1),
    proj(_proj),
    rpolar(_rpolar),
    nearest((const AbstractAirspace *)nullptr, _e.first) {}

  void Visit(const AbstractAirspace &as) override {
    assert(!intersections.empty());

    GeoPoint point = intersections[0].first;

    RouteLink l =
      rpolar.GenerateIntermediate(link.first,
                                  RoutePoint(proj.ProjectInteger(point),
                                             link.second.altitude),
                                  proj);

    if (l.second.altitude < as.GetBase().altitude ||
        l.second.altitude > as.GetTop().altitude)
      return;

    if (min_distance < 0 || l.d < min_distance) {
      min_distance = l.d;
      nearest = std::make_pair(&as, l.second);
    }
  }

  AIVResult GetNearest() const {
    return nearest;
  }
};

AirspaceRoute::RouteAirspaceIntersection
AirspaceRoute::FirstIntersecting(const RouteLink &e) const
{
  const GeoPoint origin(projection.Unproject(e.first));
  const GeoPoint dest(projection.Unproject(e.second));
  AIV visitor(e, projection, rpolars_route);
  m_airspaces.VisitIntersecting(origin, dest, visitor);
  const AIV::AIVResult res(visitor.GetNearest());
  ++count_airspace;
  return RouteAirspaceIntersection(res.first, res.second);
}

const AbstractAirspace *
AirspaceRoute::InsideOthers(const AGeoPoint &origin) const
{
  ++count_airspace;

  for (const auto &i : m_airspaces.QueryWithinRange(origin, 1))
    return &i.GetAirspace();

  return nullptr;
}


// Node generation utilities

AirspaceRoute::ClearingPair
AirspaceRoute::FindClearingPair(const SearchPointVector &spv,
                                const SearchPointVector::const_iterator start,
                                const SearchPointVector::const_iterator end,
                                const RoutePoint &dest) const
{
  bool backwards = false;
  ClearingPair p(dest, dest);

  bool check_others = false;

  SearchPointVector::const_iterator i= start;

  int j = 0;
  while (i != end && j < 2) {
    AFlatGeoPoint pborder(i->GetFlatLocation(), dest.altitude); // @todo alt!
    const FlatRay ray(pborder, dest);

    if (spv.IntersectsWith(ray)) {
      ++j;
      if (j == 1) {
        i = start;
        backwards = true;
        continue;
      }
    } else {
      AGeoPoint gborder(projection.Unproject(pborder), dest.altitude); // @todo alt!
      if (!check_others || !InsideOthers(gborder)) {
        if (j == 0) {
          p.first = pborder;
        } else if (j == 1) {
          p.second = pborder;
        }
      }
    }

    i = backwards ? spv.PreviousCircular(i) : spv.NextCircular(i);
  }

  return p;
}

AirspaceRoute::ClearingPair
AirspaceRoute::GetPairs(const SearchPointVector &spv,
                        const RoutePoint &start, const RoutePoint &dest) const
{
  SearchPointVector::const_iterator i_closest = spv.NearestIndexConvex(start);
  SearchPointVector::const_iterator i_furthest = spv.NearestIndexConvex(dest);
  return FindClearingPair(spv, i_closest, i_furthest, start);
}

AirspaceRoute::ClearingPair
AirspaceRoute::GetBackupPairs(const SearchPointVector &spv,
                              const RoutePoint &_start,
                              const RoutePoint &intc) const
{
  SearchPointVector::const_iterator start = spv.NearestIndexConvex(intc);
  ClearingPair p(intc, intc);

  SearchPointVector::const_iterator i_left = spv.NextCircular(start);
  p.first = AFlatGeoPoint(i_left->GetFlatLocation(), _start.altitude); // @todo alt!

  SearchPointVector::const_iterator i_right = spv.PreviousCircular(start);
  p.second = AFlatGeoPoint(i_right->GetFlatLocation(), _start.altitude); // @todo alt!

  return p;
}

unsigned
AirspaceRoute::AirspaceSize() const
{
  return m_airspaces.GetSize();
}

AirspaceRoute::AirspaceRoute():m_airspaces(false)
{
  Reset();
}

AirspaceRoute::~AirspaceRoute()
{
  // clean up, we dont need the clearances any more
  m_airspaces.ClearClearances();
}

void
AirspaceRoute::Reset()
{
  RoutePlanner::Reset();
  m_airspaces.ClearClearances();
  m_airspaces.Clear();
}

void
AirspaceRoute::Synchronise(const Airspaces &master,
                           const AirspacePredicate &_condition,
                           const AGeoPoint &origin,
                           const AGeoPoint &destination)
{
  // @todo: also synchronise with AirspaceWarningManager to filter out items that are
  // acknowledged.
  h_min = std::min((int)origin.altitude, std::min((int)destination.altitude, h_min));
  h_max = std::max((int)origin.altitude, std::max((int)destination.altitude, h_max));

  // @todo: have margin for h_max to allow for climb
  AirspacePredicateHeightRangeExcludeTwo h_condition(h_min, h_max, origin, destination);

  const auto and_condition = MakeAndPredicate(h_condition,
                                              AirspacePredicateRef(_condition));
  const auto predicate = WrapAirspacePredicate(and_condition);

  if (m_airspaces.SynchroniseInRange(master, origin.Middle(destination),
                                     0.5 * origin.Distance(destination),
                                     predicate)) {
    if (!m_airspaces.IsEmpty())
      dirty = true;
  }
}

void
AirspaceRoute::AddNearbyAirspace(const RouteAirspaceIntersection &inx,
                                 const RouteLink &e)
{
  const SearchPointVector &fat =
    inx.airspace->GetClearance(m_airspaces.GetProjection());
  const ClearingPair p = GetPairs(fat, e.first, e.second);
  const ClearingPair pb = GetBackupPairs(fat, e.first, inx.point);

  // process all options
  AddCandidate(RouteLinkBase(e.first, p.first));
  AddCandidate(RouteLinkBase(e.first, p.second));
  AddCandidate(RouteLinkBase(e.first, pb.first));
  AddCandidate(RouteLinkBase(e.first, pb.second));
}

void
AirspaceRoute::AddNearby(const RouteLink &e)
{
  if (m_inx.airspace == nullptr) {
    // NOTE: m_inx is "mutable" so that const in AddNearbyTerrain is ignored!!
    // The copy is really needed!
    RoutePoint ptmp = m_inx.point;
    AddNearbyTerrain(ptmp, e);
  } else
    AddNearbyAirspace(m_inx, e);
}

bool
AirspaceRoute::CheckSecondary(const RouteLink &e)
{
  if (!rpolars_route.IsAirspaceEnabled())
    return true; // trivial

  m_inx = FirstIntersecting(e);
  if (m_inx.airspace != nullptr)  {
    AddCandidate(e);
    return false;
  };
  return true;
}

bool
AirspaceRoute::CheckClearance(const RouteLink &e, RoutePoint &inp) const
{
  // attempt terrain clearance first

  if (!CheckClearanceTerrain(e, inp)) {
    m_inx.airspace = nullptr;
    m_inx.point = inp;
    return false;
  }

  if (!rpolars_route.IsAirspaceEnabled())
    return true; // trivial

  // passes terrain, so now check airspace clearance

  m_inx = FirstIntersecting(e);
  if (m_inx.airspace != nullptr)  {
    inp = m_inx.point;
    return false;
  }

  // made it this far!
  return true;
}

void
AirspaceRoute::OnSolve(const AGeoPoint &origin, const AGeoPoint &destination)
{
  if (m_airspaces.IsEmpty()) {
    projection.SetCenter(origin);
  } else {
    projection = m_airspaces.GetProjection();
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

