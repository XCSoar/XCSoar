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

#include "Waypoints.hpp"
#include "WaypointVisitor.hpp"
#include "StringUtil.hpp"

// global, used for test harness
unsigned n_queries = 0;

/**
 * Container accessor to allow a WaypointVisitor to visit WaypointEnvelopes 
 */
class WaypointEnvelopeVisitor {
public:
  /**
   * Constructor
   *
   * @param wve Contained visitor
   *
   * @return Initialised object
   */
  WaypointEnvelopeVisitor(WaypointVisitor* wve):waypoint_visitor(wve) {};

  /**
   * Accessor operator to perform visit
   */
  void
  operator()(const Waypoint &wp)
  {
    Visit(wp);
  }

  /**
   * Visit item inside envelope
   */
  void
  Visit(const Waypoint &wp)
  {
    waypoint_visitor->Visit(wp);
  }

private:
  WaypointVisitor *waypoint_visitor;
};

struct VisitorAdapter {
  WaypointVisitor &visitor;
  VisitorAdapter(WaypointVisitor &_visitor):visitor(_visitor) {}

  void operator()(const Waypoint *wp) {
    visitor.Visit(*wp);
  }
};

const Waypoint *
Waypoints::WaypointNameTree::Get(const TCHAR *name) const
{
  TCHAR normalized_name[_tcslen(name) + 1];
  normalize_search_string(normalized_name, name);
  return get(normalized_name, NULL);
}

void
Waypoints::WaypointNameTree::VisitNormalisedPrefix(const TCHAR *prefix,
                                                   WaypointVisitor &visitor) const
{
  TCHAR normalized[_tcslen(prefix) + 1];
  normalize_search_string(normalized, prefix);
  VisitorAdapter adapter(visitor);
  visit_prefix(normalized, adapter);
}

void
Waypoints::WaypointNameTree::Add(const Waypoint &wp)
{
  TCHAR normalized_name[wp.Name.length() + 1];
  normalize_search_string(normalized_name, wp.Name.c_str());
  add(normalized_name, &wp);
}

void
Waypoints::WaypointNameTree::Remove(const Waypoint &wp)
{
  TCHAR normalized_name[wp.Name.length() + 1];
  normalize_search_string(normalized_name, wp.Name.c_str());
  remove(normalized_name, &wp);
}

Waypoints::Waypoints():
  next_id(1),
  m_home(NULL)
{
}

class LandablePredicate {
public:
  bool operator()(const Waypoint &wp) const {
    return wp.IsLandable();
  }
};

void
Waypoints::optimise()
{
  if (waypoint_tree.IsEmpty())
    return;

  task_projection.update_fast();

  for (WaypointTree::iterator it = waypoint_tree.begin();
       it != waypoint_tree.end(); ++it)
    it->Project(task_projection);

  waypoint_tree.Optimise();
}

const Waypoint &
Waypoints::append(const Waypoint &_wp)
{
  Waypoint wp(_wp);

  if (empty())
    task_projection.reset(wp.Location);
  else if (waypoint_tree.HaveBounds())
    /* QuadTree::IsWithinKnownBounds() needs to know the position */
    wp.Project(task_projection);

  wp.Flags.Watched = (wp.FileNum == 3);

  task_projection.scan_location(wp.Location);
  wp.id = next_id++;

  bool must_optimise = !waypoint_tree.IsWithinKnownBounds(wp);
  if (must_optimise) {
    waypoint_tree.Flatten();
    waypoint_tree.ClearBounds();
  }

  const Waypoint &new_wp = waypoint_tree.Add(wp);

  if (must_optimise)
    waypoint_tree.Optimise();

  name_tree.Add(new_wp);

  return new_wp;
}

const Waypoint*
Waypoints::get_nearest(const GeoPoint &loc, fixed range) const
{
  if (empty())
    return NULL;

  Waypoint bb_target(loc);
  bb_target.Project(task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);
  std::pair<WaypointTree::const_iterator, WaypointTree::distance_type> found =
    waypoint_tree.FindNearest(bb_target, mrange);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  if (found.first == waypoint_tree.end())
    return NULL;

  return &*found.first;
}

const Waypoint*
Waypoints::get_nearest_landable(const GeoPoint &loc, fixed range) const
{
  if (empty())
    return NULL;

  Waypoint bb_target(loc);
  bb_target.Project(task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);
  std::pair<WaypointTree::const_iterator, WaypointTree::distance_type> found =
      waypoint_tree.FindNearestIf(bb_target, mrange, LandablePredicate());

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif

  if (found.first == waypoint_tree.end())
    return NULL;

  return &*found.first;
}

const Waypoint*
Waypoints::lookup_name(const TCHAR *name) const
{
  return name_tree.Get(name);
}

const Waypoint*
Waypoints::lookup_location(const GeoPoint &loc, const fixed range) const
{
  const Waypoint* wp = get_nearest(loc, range);
  if (!wp)
    return NULL;

  if (wp->Location == loc)
    return wp;
  else if (positive(range) && (wp->IsCloseTo(loc, range)))
    return wp;

  return NULL;
}

const Waypoint*
Waypoints::find_home()
{
  for (const_iterator found = waypoint_tree.begin();
       found != waypoint_tree.end(); ++found) {
    const Waypoint &wp = *found;
    if (wp.Flags.Home) {
      m_home = &wp;
      return &wp;
    }
  }

  return NULL;
}

bool
Waypoints::set_home(const unsigned id)
{
  m_home = NULL;

  for (WaypointTree::iterator found = waypoint_tree.begin();
       found != waypoint_tree.end(); ++found) {
    Waypoint &wp = *found;

    if (wp.id == id) {
      m_home = &wp;
      wp.Flags.Home = true;
      return true;
    }
  }

  return false;
}

const Waypoint*
Waypoints::lookup_id(const unsigned id) const
{
  for (const_iterator found = waypoint_tree.begin();
       found != waypoint_tree.end(); ++found)
    if (found->id == id)
      return &*found;

  return NULL;
}

void
Waypoints::visit_within_range(const GeoPoint &loc, const fixed range,
    WaypointVisitor& visitor) const
{
  if (empty())
    return; // nothing to do

  Waypoint bb_target(loc);
  bb_target.Project(task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);

  WaypointEnvelopeVisitor wve(&visitor);

  waypoint_tree.VisitWithinRange(bb_target, mrange, wve);

#ifdef INSTRUMENT_TASK
  n_queries++;
#endif
}

void
Waypoints::visit_name_prefix(const TCHAR *prefix,
                             WaypointVisitor& visitor) const
{
  name_tree.VisitNormalisedPrefix(prefix, visitor);
}

void
Waypoints::clear()
{
  m_home = NULL;
  name_tree.clear();
  waypoint_tree.clear();
  next_id = 1;
}

void
Waypoints::erase(const Waypoint& wp)
{
  if (m_home != NULL && m_home->id == wp.id)
    m_home = NULL;

  WaypointTree::const_iterator it = waypoint_tree.FindPointer(&wp);
  assert(it != waypoint_tree.end());

  name_tree.Remove(wp);
  waypoint_tree.erase(it);
}

void
Waypoints::replace(const Waypoint &orig, const Waypoint &replacement)
{
  name_tree.Remove(orig);

  Waypoint new_waypoint(replacement);
  new_waypoint.id = orig.id;
  new_waypoint.Project(task_projection);

  bool must_optimise = !waypoint_tree.IsWithinKnownBounds(new_waypoint);
  if (must_optimise) {
    waypoint_tree.Flatten();
    waypoint_tree.ClearBounds();
  }

  WaypointTree::const_iterator it = waypoint_tree.FindPointer(&orig);
  assert(it != waypoint_tree.end());
  waypoint_tree.Replace(it, new_waypoint);

  if (must_optimise)
    waypoint_tree.Optimise();

  name_tree.Add(orig);
}

Waypoint
Waypoints::create(const GeoPoint &location)
{
  Waypoint edit_waypoint(location);

  // first waypoint, put into primary file (this will be auto-generated)
  edit_waypoint.FileNum = 1;
  edit_waypoint.original_id = 0;
  return edit_waypoint;
}

bool
Waypoints::check_exists_or_append(Waypoint &waypoint)
{
  const Waypoint* found = lookup_name(waypoint.Name);
  if (found && found->IsCloseTo(waypoint.Location, fixed(100))) {
    waypoint = *found;
    return true;
  }
  waypoint = append(waypoint);
  return false;
}

Waypoint 
Waypoints::generate_takeoff_point(const GeoPoint& location,
                                  const fixed terrain_alt) const
{
  // fallback: create a takeoff point
  Waypoint to_point(location, true);
  to_point.Altitude = terrain_alt;
  to_point.FileNum = -1;
  to_point.Name = _T("(takeoff)");
  to_point.Type = Waypoint::wtOutlanding;
  return to_point;
}

void 
Waypoints::add_takeoff_point(const GeoPoint& location,
                             const fixed terrain_alt)
{
  // remove old one first
  const Waypoint *old_takeoff_point = lookup_name(_T("(takeoff)"));
  if (old_takeoff_point != NULL)
    erase(*old_takeoff_point);

  const Waypoint *nearest_landable = get_nearest_landable(location,
                                                          fixed(5000));
  if (!nearest_landable) {
    // now add new and update database
    Waypoint new_waypoint = generate_takeoff_point(location, terrain_alt);
    append(new_waypoint);
  }

  optimise();
}
