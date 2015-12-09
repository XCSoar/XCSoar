/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceVisitor.hpp"
#include "AirspaceIntersectionVisitor.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Navigation/Aircraft.hpp"
#include "Geo/Flat/FlatRay.hpp"
#include "Geo/Flat/TaskProjection.hpp"

#include <functional>

#ifdef INSTRUMENT_TASK
extern unsigned n_queries;
extern long count_intersections;
#endif

class AirspacePredicateVisitorAdapter {
  const AirspacePredicate *predicate;
  AirspaceVisitor *visitor;

public:
  AirspacePredicateVisitorAdapter(const AirspacePredicate &_predicate,
                                  AirspaceVisitor &_visitor)
    :predicate(&_predicate), visitor(&_visitor) {}

  void operator()(Airspace as) {
    AbstractAirspace &aas = as.GetAirspace();
    if (predicate->operator()(aas))
      visitor->Visit(aas);
  }
};

void
Airspaces::VisitWithinRange(const GeoPoint &location, fixed range,
                            AirspaceVisitor &visitor,
                            const AirspacePredicate &predicate) const
{
  if (IsEmpty())
    // nothing to do
    return;

  Airspace bb_target(location, task_projection);
  int projected_range = task_projection.ProjectRangeInteger(location, range);
  AirspacePredicateVisitorAdapter adapter(predicate, visitor);
  airspace_tree.visit_within_range(bb_target, -projected_range, adapter);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif
}

class IntersectingAirspaceVisitorAdapter {
  GeoPoint start, end;
  const FlatProjection *projection;
  FlatRay ray;
  AirspaceIntersectionVisitor *visitor;

public:
  IntersectingAirspaceVisitorAdapter(const GeoPoint &_loc,
                                     const GeoPoint &_end,
                                     const FlatProjection &_projection,
                                     AirspaceIntersectionVisitor &_visitor)
    :start(_loc), end(_end), projection(&_projection),
     ray(projection->ProjectInteger(start), projection->ProjectInteger(end)),
     visitor(&_visitor) {}

  void operator()(const Airspace &as) {
    if (as.Intersects(ray) &&
        visitor->SetIntersections(as.Intersects(start, end, *projection)))
      visitor->Visit(as.GetAirspace());
  }
};

void
Airspaces::VisitIntersecting(const GeoPoint &loc, const GeoPoint &end,
                             AirspaceIntersectionVisitor &visitor) const
{
  if (IsEmpty())
    // nothing to do
    return;

  const GeoPoint c = loc.Middle(end);
  Airspace bb_target(c, task_projection);
  int projected_range = task_projection.ProjectRangeInteger(c, loc.Distance(end) / 2);
  IntersectingAirspaceVisitorAdapter adapter(loc, end, task_projection, visitor);
  airspace_tree.visit_within_range(bb_target, -projected_range, adapter);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif
}

// SCAN METHODS

Airspaces::AirspaceVector
Airspaces::ScanRange(const GeoPoint &location, fixed range,
                     const AirspacePredicate &condition) const
{
  if (IsEmpty())
    // nothing to do
    return AirspaceVector();

  Airspace bb_target(location, task_projection);
  int projected_range = task_projection.ProjectRangeInteger(location, range);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  AirspaceVector res;

  std::function<void(const Airspace &)> visitor =
    [&location, range, projected_range,
     &condition, &bb_target, &res](const Airspace &v){
    if (condition(v.GetAirspace()) &&
        v.Distance(bb_target) <= unsigned(projected_range) &&
        (v.IsInside(location) || positive(range)))
      res.push_back(v);
  };

  airspace_tree.visit_within_range(bb_target, -projected_range, visitor);

  return res;
}

Airspaces::AirspaceVector
Airspaces::FindInside(const AircraftState &state,
                      const AirspacePredicate &condition) const
{
  Airspace bb_target(state.location, task_projection);

  AirspaceVector vectors;

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  std::function<void(const Airspace &)> visitor =
    [&state, &condition, &vectors](const Airspace &v){

#ifdef INSTRUMENT_TASK
    count_intersections++;
#endif

    if (condition(v.GetAirspace()) &&
        v.IsInside(state))
      vectors.push_back(v);
  };

  airspace_tree.visit_within_range(bb_target, 0, visitor);

  return vectors;
}

void
Airspaces::Optimise()
{
  if (IsEmpty())
    /* avoid assertion failure in uninitialised task_projection */
    return;

  if (!owns_children || task_projection.Update()) {
    // dont update task_projection if not owner!

    // task projection changed, so need to push items back onto stack
    // to re-build airspace envelopes

    for (const auto &i : airspace_tree)
      tmp_as.push_back(&i.GetAirspace());

    airspace_tree.clear();
  }

  if (!tmp_as.empty()) {
    while (!tmp_as.empty()) {
      Airspace as(*tmp_as.front(), task_projection);
      airspace_tree.insert(as);
      tmp_as.pop_front();
    }
    airspace_tree.optimise();
  }

  ++serial;
}

void
Airspaces::Add(AbstractAirspace *airspace)
{
  if (!airspace)
    // nothing to add
    return;

  // reset QNH to zero so set_pressure_levels will be triggered next update
  // this allows for airspaces to be add at any time
  qnh = AtmosphericPressure::Zero();

  // reset day to all so set_activity will be triggered next update
  // this allows for airspaces to be add at any time
  activity_mask.SetAll();

  if (owns_children) {
    if (IsEmpty())
      task_projection.Reset(airspace->GetReferenceLocation());

    task_projection.Scan(airspace->GetReferenceLocation());
  }

  tmp_as.push_back(airspace);
}

void
Airspaces::Clear()
{
  // delete temporaries in case they were added without an optimise() call
  while (!tmp_as.empty()) {
    if (owns_children) {
      AbstractAirspace *aa = tmp_as.front();
      delete aa;
    }
    tmp_as.pop_front();
  }

  // delete items in the tree
  if (owns_children) {
    for (const auto &i : airspace_tree) {
      Airspace a = i;
      a.Destroy();
    }
  }

  // then delete the tree
  airspace_tree.clear();
}

unsigned
Airspaces::GetSize() const
{
  return airspace_tree.size();
}

bool
Airspaces::IsEmpty() const
{
  return airspace_tree.empty() && tmp_as.empty();
}

void
Airspaces::SetFlightLevels(const AtmosphericPressure &press)
{
  if ((int)press.GetHectoPascal() != (int)qnh.GetHectoPascal()) {
    qnh = press;

    for (auto &v : airspace_tree)
      v.SetFlightLevel(press);
  }
}

void
Airspaces::SetActivity(const AirspaceActivity mask)
{
  if (!mask.equals(activity_mask)) {
    activity_mask = mask;

    for (auto &v : airspace_tree)
      v.SetActivity(mask);
  }
}

void
Airspaces::ClearClearances()
{
  for (auto &v : airspace_tree)
    v.ClearClearance();
}

gcc_pure
static bool
AirspacePointersLess(const Airspace &a, const Airspace &b)
{
  return &a.GetAirspace() < &b.GetAirspace();
}

gcc_pure
static AirspacesInterface::AirspaceVector
SortByPointer(AirspacesInterface::AirspaceVector &&v)
{
  std::sort(v.begin(), v.end(), AirspacePointersLess);
  return v;
}

gcc_pure
static bool
AirspacePointersEquals(const Airspace &a, const Airspace &b)
{
  return &a.GetAirspace() == &b.GetAirspace();
}

gcc_pure
static bool
CompareSortedAirspaceVectors(const AirspacesInterface::AirspaceVector &a,
                             const AirspacesInterface::AirspaceVector &b)
{
  return a.size() == b.size() &&
    std::equal(a.begin(), a.end(), b.begin(), AirspacePointersEquals);
}

inline AirspacesInterface::AirspaceVector
Airspaces::AsVector() const
{
  AirspaceVector v;
  v.reserve(airspace_tree.size());

  for (const auto &i : airspace_tree)
    v.push_back(i);

  return v;
}

bool
Airspaces::SynchroniseInRange(const Airspaces &master,
                              const GeoPoint &location,
                              const fixed range,
                              const AirspacePredicate &condition)
{
  qnh = master.qnh;
  activity_mask = master.activity_mask;
  task_projection = master.task_projection;

  const AirspaceVector contents_master =
    SortByPointer(master.ScanRange(location, range, condition));

  if (CompareSortedAirspaceVectors(contents_master, SortByPointer(AsVector())))
    return false;

  for (auto &i : airspace_tree)
    i.ClearClearance();
  airspace_tree.clear();

  for (const auto &i : contents_master)
    airspace_tree.insert(i);

  airspace_tree.optimise();

  ++serial;

  return true;
}

void
Airspaces::VisitInside(const GeoPoint &loc, AirspaceVisitor &visitor) const
{
  if (IsEmpty())
    // nothing to do
    return;

  Airspace bb_target(loc, task_projection);

  std::function<void(const Airspace &)> visitor2 =
    [&loc, &visitor](const Airspace &v){
    const AbstractAirspace &as = v.GetAirspace();
    if (as.Inside(loc))
      visitor.Visit(as);
  };

  airspace_tree.visit_within_range(bb_target, 0, visitor2);
}
