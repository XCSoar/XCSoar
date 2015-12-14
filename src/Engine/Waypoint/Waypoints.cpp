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

#include "Waypoints.hpp"
#include "WaypointVisitor.hpp"
#include "Util/StringUtil.hpp"

// global, used for test harness
unsigned n_queries = 0;

/**
 * Container accessor to allow a WaypointVisitor to visit
 * WaypointEnvelopes.
 */
class WaypointEnvelopeVisitor {
  WaypointVisitor *const waypoint_visitor;

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
  NormalizeSearchString(normalized_name, name);
  return RadixTree<const Waypoint *>::Get(normalized_name, nullptr);
}

void
Waypoints::WaypointNameTree::VisitNormalisedPrefix(const TCHAR *prefix,
                                                   WaypointVisitor &visitor) const
{
  TCHAR normalized[_tcslen(prefix) + 1];
  NormalizeSearchString(normalized, prefix);
  VisitorAdapter adapter(visitor);
  VisitPrefix(normalized, adapter);
}

TCHAR *
Waypoints::WaypointNameTree::SuggestNormalisedPrefix(const TCHAR *prefix,
                                                     TCHAR *dest,
                                                     size_t max_length) const
{
  TCHAR normalized[_tcslen(prefix) + 1];
  NormalizeSearchString(normalized, prefix);
  return Suggest(normalized, dest, max_length);
}

void
Waypoints::WaypointNameTree::Add(const Waypoint &wp)
{
  TCHAR normalized_name[wp.name.length() + 1];
  NormalizeSearchString(normalized_name, wp.name.c_str());
  RadixTree<const Waypoint *>::Add(normalized_name, &wp);
}

void
Waypoints::WaypointNameTree::Remove(const Waypoint &wp)
{
  TCHAR normalized_name[wp.name.length() + 1];
  NormalizeSearchString(normalized_name, wp.name.c_str());
  RadixTree<const Waypoint *>::Remove(normalized_name, &wp);
}

Waypoints::Waypoints()
  :next_id(1),
   home(nullptr)
{
}

void
Waypoints::Optimise()
{
  if (waypoint_tree.IsEmpty() || waypoint_tree.HaveBounds())
    /* empty or already optimised */
    return;

  task_projection.Update();

  for (auto &i : waypoint_tree)
    i.Project(task_projection);

  waypoint_tree.Optimise();
}

const Waypoint &
Waypoints::Append(Waypoint &&wp)
{
  if (waypoint_tree.HaveBounds()) {
    wp.Project(task_projection);
    if (!waypoint_tree.IsWithinBounds(wp))
      ScheduleOptimise();
  } else if (IsEmpty())
    task_projection.Reset(wp.location);

  wp.flags.watched = wp.origin == WaypointOrigin::WATCHED;

  task_projection.Scan(wp.location);
  wp.id = next_id++;

  const Waypoint &new_wp = waypoint_tree.Add(std::move(wp));
  name_tree.Add(new_wp);

  ++serial;

  return new_wp;
}

const Waypoint *
Waypoints::GetNearest(const GeoPoint &loc, fixed range) const
{
  if (IsEmpty())
    return nullptr;

  const FlatGeoPoint flat_location = task_projection.ProjectInteger(loc);
  const WaypointTree::Point point(flat_location.x, flat_location.y);
  const unsigned mrange = task_projection.ProjectRangeInteger(loc, range);
  const auto found = waypoint_tree.FindNearest(point, mrange);

  if (found.first == waypoint_tree.end())
    return nullptr;

  return &*found.first;
}

static bool
IsLandable(const Waypoint &wp)
{
  return wp.IsLandable();
}

const Waypoint *
Waypoints::GetNearestLandable(const GeoPoint &loc, fixed range) const
{
  return GetNearestIf(loc, range, IsLandable);
}

const Waypoint *
Waypoints::GetNearestIf(const GeoPoint &loc, fixed range,
                        bool (*predicate)(const Waypoint &)) const
{
  if (IsEmpty())
    return nullptr;

  const FlatGeoPoint flat_location = task_projection.ProjectInteger(loc);
  const WaypointTree::Point point(flat_location.x, flat_location.y);
  const unsigned mrange = task_projection.ProjectRangeInteger(loc, range);
  const auto found = waypoint_tree.FindNearestIf(point, mrange, predicate);

  if (found.first == waypoint_tree.end())
    return nullptr;

  return &*found.first;
}

const Waypoint *
Waypoints::LookupName(const TCHAR *name) const
{
  return name_tree.Get(name);
}

const Waypoint *
Waypoints::LookupLocation(const GeoPoint &loc, const fixed range) const
{
  const Waypoint *wp = GetNearest(loc, range);
  if (!wp)
    return nullptr;

  if (wp->location == loc)
    return wp;
  else if (positive(range) && (wp->IsCloseTo(loc, range)))
    return wp;

  return nullptr;
}

const Waypoint *
Waypoints::FindHome()
{
  for (const auto &wp : waypoint_tree) {
    if (wp.flags.home) {
      home = &wp;
      return &wp;
    }
  }

  return nullptr;
}

bool
Waypoints::SetHome(const unsigned id)
{
  home = LookupId(id);
  if (home == nullptr)
    return false;

  Waypoint &wp = const_cast<Waypoint &>(*home);
  wp.flags.home = true;
  return true;
}

const Waypoint *
Waypoints::LookupId(const unsigned id) const
{
  for (const auto &wp : waypoint_tree)
    if (wp.id == id)
      return &wp;

  return nullptr;
}

void
Waypoints::VisitWithinRange(const GeoPoint &loc, const fixed range,
    WaypointVisitor& visitor) const
{
  if (IsEmpty())
    return; // nothing to do

  const FlatGeoPoint flat_location = task_projection.ProjectInteger(loc);
  const WaypointTree::Point point(flat_location.x, flat_location.y);
  const unsigned mrange = task_projection.ProjectRangeInteger(loc, range);

  WaypointEnvelopeVisitor wve(&visitor);

  waypoint_tree.VisitWithinRange(point, mrange, wve);
}

void
Waypoints::VisitNamePrefix(const TCHAR *prefix,
                           WaypointVisitor& visitor) const
{
  name_tree.VisitNormalisedPrefix(prefix, visitor);
}

void
Waypoints::Clear()
{
  ++serial;
  home = nullptr;
  name_tree.Clear();
  waypoint_tree.clear();
  next_id = 1;
}

void
Waypoints::Erase(const Waypoint& wp)
{
  if (home == &wp)
    home = nullptr;

  const auto it = waypoint_tree.FindPointer(&wp);
  assert(it != waypoint_tree.end());

  name_tree.Remove(wp);
  waypoint_tree.erase(it);
  ++serial;
}

void
Waypoints::EraseUserMarkers()
{
  waypoint_tree.EraseIf([this](const Waypoint &wp){
      if (wp.origin == WaypointOrigin::USER &&
          wp.type == Waypoint::Type::MARKER) {
        if (home == &wp)
          home = nullptr;

        name_tree.Remove(wp);
        ++serial;
        return true;
      } else
        return false;
    });
}

void
Waypoints::Replace(const Waypoint &orig, const Waypoint &replacement)
{
  assert(!waypoint_tree.IsEmpty());

  name_tree.Remove(orig);

  Waypoint new_waypoint(replacement);
  new_waypoint.id = orig.id;

  if (waypoint_tree.HaveBounds()) {
    new_waypoint.Project(task_projection);
    if (!waypoint_tree.IsWithinBounds(new_waypoint))
      ScheduleOptimise();
  }

  const auto it = waypoint_tree.FindPointer(&orig);
  assert(it != waypoint_tree.end());
  waypoint_tree.Replace(it, new_waypoint);

  name_tree.Add(orig);
  ++serial;
}

Waypoint
Waypoints::Create(const GeoPoint &location)
{
  Waypoint edit_waypoint(location);

  // first waypoint, put into primary file (this will be auto-generated)
  edit_waypoint.origin = WaypointOrigin::USER;
  edit_waypoint.original_id = 0;
  return edit_waypoint;
}

const Waypoint &
Waypoints::CheckExistsOrAppend(const Waypoint &waypoint)
{
  const Waypoint *found = LookupName(waypoint.name);
  if (found && found->IsCloseTo(waypoint.location, fixed(100))) {
    return *found;
  }

  return Append(Waypoint(waypoint));
}

Waypoint
Waypoints::GenerateTakeoffPoint(const GeoPoint& location,
                                  const fixed terrain_alt) const
{
  // fallback: create a takeoff point
  Waypoint to_point(location);
  to_point.elevation = terrain_alt;
  to_point.name = _T("(takeoff)");
  to_point.type = Waypoint::Type::OUTLANDING;
  return to_point;
}

void
Waypoints::AddTakeoffPoint(const GeoPoint& location,
                             const fixed terrain_alt)
{
  // remove old one first
  const Waypoint *old_takeoff_point = LookupName(_T("(takeoff)"));
  if (old_takeoff_point != nullptr)
    Erase(*old_takeoff_point);

  const Waypoint *nearest_landable = GetNearestLandable(location,
                                                          fixed(5000));
  if (!nearest_landable) {
    // now add new and update database
    Waypoint new_waypoint = GenerateTakeoffPoint(location, terrain_alt);
    Append(std::move(new_waypoint));
  }

  Optimise();
}
