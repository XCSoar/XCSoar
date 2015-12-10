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

#include <boost/geometry/geometries/linestring.hpp>

#include <functional>

namespace bgi = boost::geometry::index;

struct AirspacePredicateAdapter {
  const AirspacePredicate &predicate;

  bool operator()(const Airspace &as) const {
    return predicate(as.GetAirspace());
  }
};

Airspaces::const_iterator_range
Airspaces::QueryWithinRange(const GeoPoint &location, fixed range) const
{
  if (IsEmpty())
    // nothing to do
    return {airspace_tree.qend(), airspace_tree.qend()};

  const auto flat_location = task_projection.ProjectInteger(location);
  int projected_range = task_projection.ProjectRangeInteger(location, range);
  const FlatBoundingBox box(flat_location, projected_range);

  return {airspace_tree.qbegin(bgi::intersects(box)), airspace_tree.qend()};
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

  // TODO: use StaticArray instead of std::vector
  boost::geometry::model::linestring<FlatGeoPoint> line;
  line.push_back(task_projection.ProjectInteger(loc));
  line.push_back(task_projection.ProjectInteger(end));

  IntersectingAirspaceVisitorAdapter adapter(loc, end, task_projection, visitor);

  const auto _begin =
    airspace_tree.qbegin(bgi::intersects(line));
  const auto _end = airspace_tree.qend();

  for (auto i = _begin; i != _end; ++i)
    adapter(*i);
}

// SCAN METHODS

Airspaces::AirspaceVector
Airspaces::ScanRange(const GeoPoint &location, fixed range,
                     const AirspacePredicate &condition) const
{
  if (IsEmpty())
    // nothing to do
    return AirspaceVector();

  const auto flat_location = task_projection.ProjectInteger(location);
  unsigned projected_range = task_projection.ProjectRangeInteger(location,
                                                                 range);
  const FlatBoundingBox box(flat_location, projected_range);

  auto predicate = bgi::intersects(box) &&
    bgi::satisfies(AirspacePredicateAdapter{condition}) &&
    bgi::satisfies([&box, projected_range, &location, range](const Airspace &a){
        return a.Distance(box) <= projected_range &&
        (a.IsInside(location) || positive(range));
      });

  AirspaceVector res;
  airspace_tree.query(std::move(predicate), std::back_inserter(res));
  return res;
}

Airspaces::AirspaceVector
Airspaces::FindInside(const AircraftState &state,
                      const AirspacePredicate &condition) const
{
  const auto flat_location = task_projection.ProjectInteger(state.location);
  const FlatBoundingBox box(flat_location, flat_location);

  AirspaceVector vectors;

  auto predicate = bgi::intersects(box) &&
    bgi::satisfies(AirspacePredicateAdapter{condition}) &&
    bgi::satisfies([&state](const Airspace &a){
        return a.IsInside(state);
      });

  airspace_tree.query(std::move(predicate), std::back_inserter(vectors));
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

    for (const auto &i : *this)
      tmp_as.push_back(&i.GetAirspace());

    airspace_tree.clear();
  }

  if (!tmp_as.empty()) {
    while (!tmp_as.empty()) {
      Airspace as(*tmp_as.front(), task_projection);
      airspace_tree.insert(as);
      tmp_as.pop_front();
    }
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
    for (const auto &i : *this) {
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

    for (auto &v : *this)
      v.SetFlightLevel(press);
  }
}

void
Airspaces::SetActivity(const AirspaceActivity mask)
{
  if (!mask.equals(activity_mask)) {
    activity_mask = mask;

    for (auto &v : *this)
      v.SetActivity(mask);
  }
}

void
Airspaces::ClearClearances()
{
  for (auto &v : *this)
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

  for (const auto &i : *this)
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

  for (auto &i : *this)
    i.ClearClearance();
  airspace_tree.clear();

  for (const auto &i : contents_master)
    airspace_tree.insert(i);

  ++serial;

  return true;
}

Airspaces::const_iterator_range
Airspaces::QueryInside(const GeoPoint &loc) const
{
  if (IsEmpty())
    // nothing to do
    return {airspace_tree.qend(), airspace_tree.qend()};

  const auto flat_location = task_projection.ProjectInteger(loc);
  const FlatBoundingBox box(flat_location, flat_location);

  const auto _begin =
    airspace_tree.qbegin(bgi::intersects(box) &&
                         bgi::satisfies([&loc](const Airspace &as){
                             return as.IsInside(loc);
                           }));

  return {_begin, airspace_tree.qend()};
}
