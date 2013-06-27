/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
    AbstractAirspace &aas = *as.GetAirspace();
    if (predicate->operator()(aas))
      visitor->Visit(as);
  }
};

void 
Airspaces::VisitWithinRange(const GeoPoint &location, fixed range,
                            AirspaceVisitor& visitor,
                            const AirspacePredicate &predicate) const
{
  if (empty())
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
  const TaskProjection *projection;
  FlatRay ray;
  AirspaceIntersectionVisitor *visitor;

public:
  IntersectingAirspaceVisitorAdapter(const GeoPoint &_loc,
                                     const GeoPoint &_end,
                                     const TaskProjection &_projection,
                                     AirspaceIntersectionVisitor &_visitor)
    :start(_loc), end(_end), projection(&_projection),
     ray(projection->ProjectInteger(start), projection->ProjectInteger(end)),
     visitor(&_visitor) {}

  void operator()(Airspace as) {
    if (as.Intersects(ray) &&
        visitor->SetIntersections(as.Intersects(start, end, *projection)))
      visitor->Visit(as);
  }
};

void 
Airspaces::VisitIntersecting(const GeoPoint &loc, const GeoPoint &end,
                             AirspaceIntersectionVisitor& visitor) const
{
  if (empty())
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

struct AirspacePredicateAdapter {
  const AirspacePredicate &condition;

  AirspacePredicateAdapter(const AirspacePredicate &_condition)
    :condition(_condition) {}

  bool operator()(const Airspace &as) const {
    return condition(*as.GetAirspace());
  }
};

const Airspace *
Airspaces::FindNearest(const GeoPoint &location,
                       const AirspacePredicate &condition) const
{
  if (empty())
    return NULL;

  const Airspace bb_target(location, task_projection);
  const int projected_range =
    task_projection.ProjectRangeInteger(location, fixed(30000));
  const AirspacePredicateAdapter predicate(condition);
  std::pair<AirspaceTree::const_iterator, AirspaceTree::distance_type> found =
    airspace_tree.find_nearest_if(bb_target, BBDist(0, projected_range),
                                  predicate);

  return found.first != airspace_tree.end() ? &*found.first : NULL;
}

const Airspaces::AirspaceVector
Airspaces::ScanRange(const GeoPoint &location, fixed range,
                     const AirspacePredicate &condition) const
{
  if (empty())
    // nothing to do
    return AirspaceVector();

  Airspace bb_target(location, task_projection);
  int projected_range = task_projection.ProjectRangeInteger(location, range);
  
  std::deque< Airspace > vectors;
  airspace_tree.find_within_range(bb_target, -projected_range,
                                  std::back_inserter(vectors));

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  AirspaceVector res;

  for (const auto &v : vectors) {
    if (!condition(*v.GetAirspace()))
      continue;

    if (fixed(v.Distance(bb_target)) > range)
      continue;

    if (v.IsInside(location) || positive(range))
      res.push_back(v);
  }

  return res;
}

const Airspaces::AirspaceVector
Airspaces::FindInside(const AircraftState &state,
                      const AirspacePredicate &condition) const
{
  Airspace bb_target(state.location, task_projection);

  AirspaceVector vectors;
  airspace_tree.find_within_range(bb_target, 0, std::back_inserter(vectors));

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  for (auto v = vectors.begin(); v != vectors.end();) {

#ifdef INSTRUMENT_TASK
    count_intersections++;
#endif
    
    if (!condition(*v->GetAirspace()) || !(*v).IsInside(state))
      v = vectors.erase(v);
    else
      ++v;
  }

  return vectors;
}

void 
Airspaces::Optimise()
{
  if (!owns_children || task_projection.Update()) {
    // dont update task_projection if not owner!

    // task projection changed, so need to push items back onto stack
    // to re-build airspace envelopes

    for (const auto &i : airspace_tree)
      tmp_as.push_back(i.GetAirspace());

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
    if (empty())
      task_projection.Reset(airspace->GetCenter());

    task_projection.Scan(airspace->GetCenter());
  }

  tmp_as.push_back(airspace);
}

void
Airspaces::clear()
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
Airspaces::size() const
{
  return airspace_tree.size();
}

bool
Airspaces::empty() const
{
  return airspace_tree.empty() && tmp_as.empty();
}

Airspaces::~Airspaces()
{
  clear();
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

Airspaces::Airspaces(const Airspaces& master, bool _owns_children):
  qnh(master.qnh),
  activity_mask(master.activity_mask),
  owns_children(_owns_children),
  task_projection(master.task_projection)
{
}

void
Airspaces::ClearClearances()
{
  for (auto &v : airspace_tree)
    v.ClearClearance();
}


bool
Airspaces::SynchroniseInRange(const Airspaces& master,
                                const GeoPoint &location,
                                const fixed range,
                                const AirspacePredicate &condition)
{
  bool changed = false;
  const AirspaceVector contents_master = master.ScanRange(location, range, condition);
  AirspaceVector contents_self;
  contents_self.reserve(std::max(airspace_tree.size(), contents_master.size()));

  task_projection = master.task_projection; // ensure these are up to date

  for (const auto &v : airspace_tree)
    contents_self.push_back(v);

  // find items to add
  for (const auto &v : contents_master) {
    const AbstractAirspace* other = v.GetAirspace();

    bool found = false;
    for (auto s = contents_self.begin(); s != contents_self.end(); ++s) {
      const AbstractAirspace* self = s->GetAirspace();
      if (self == other) {
        found = true;
        contents_self.erase(s);
        break;
      }
    }
    if (!found && other->IsActive()) {
      Add(v.GetAirspace());
      changed = true;
    }
  }

  // anything left in the self list are items that were not in the query,
  // so delete them --- including the clearances!
  for (auto v = contents_self.begin(); v != contents_self.end();) {
    gcc_unused bool found = false;
    for (auto t = airspace_tree.begin(); t != airspace_tree.end(); ) {
      if (t->GetAirspace() == v->GetAirspace()) {
        AirspaceTree::const_iterator new_t = t;
        ++new_t;
        airspace_tree.erase_exact(*t);
        t = new_t;
        found = true;
      } else {
        ++t;
      }
    }
    assert(found);
    v->ClearClearance();
    v = contents_self.erase(v);
    changed = true;
  }
  if (changed)
    Optimise();
  return changed;
}

void
Airspaces::VisitInside(const GeoPoint &loc,
                        AirspaceVisitor& visitor) const
{
  if (empty()) return; // nothing to do

  Airspace bb_target(loc, task_projection);
  AirspaceVector vectors;
  airspace_tree.find_within_range(bb_target, 0, std::back_inserter(vectors));

  for (auto &v : vectors)
    if (v.IsInside(loc))
      visitor.Visit(v);
}

