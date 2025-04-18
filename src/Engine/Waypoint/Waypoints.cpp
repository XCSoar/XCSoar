// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Waypoints.hpp"
#include "util/AllocatedArray.hxx"
#include "util/StringUtil.hpp"

static constexpr std::size_t NORMALIZE_BUFFER_SIZE = 4096;

inline WaypointPtr
Waypoints::WaypointNameTree::Get(tstring_view name) const noexcept
{
  if (name.size() >= NORMALIZE_BUFFER_SIZE)
    return {};

  TCHAR normalized_name[NORMALIZE_BUFFER_SIZE];
  NormalizeSearchString(normalized_name, name);
  return RadixTree<WaypointPtr>::Get(normalized_name, nullptr);
}

inline void
Waypoints::WaypointNameTree::VisitNormalisedPrefix(tstring_view prefix,
                                                   const WaypointVisitor &visitor) const
{
  if (prefix.size() >= NORMALIZE_BUFFER_SIZE)
    return;

  TCHAR normalized[NORMALIZE_BUFFER_SIZE];
  NormalizeSearchString(normalized, prefix);
  VisitPrefix(normalized, visitor);
}

TCHAR *
Waypoints::WaypointNameTree::SuggestNormalisedPrefix(tstring_view prefix,
                                                     TCHAR *dest,
                                                     size_t max_length) const noexcept
{
  if (prefix.size() >= NORMALIZE_BUFFER_SIZE)
    return nullptr;

  TCHAR normalized[NORMALIZE_BUFFER_SIZE];
  NormalizeSearchString(normalized, prefix);
  return Suggest(normalized, dest, max_length);
}

inline void
Waypoints::WaypointNameTree::Add(WaypointPtr wp) noexcept
{
  AllocatedArray<TCHAR> buffer(wp->name.length() + 1);
  NormalizeSearchString(buffer.data(), wp->name);
  RadixTree<WaypointPtr>::Add(buffer.data(), wp);

  if (!wp->shortname.empty()) {
    buffer.GrowDiscard(wp->shortname.length() + 1);
    NormalizeSearchString(buffer.data(), wp->shortname);
    RadixTree<WaypointPtr>::Add(buffer.data(), std::move(wp));
  }
}

inline void
Waypoints::WaypointNameTree::Remove(const WaypointPtr &wp) noexcept
{
  AllocatedArray<TCHAR> buffer(wp->name.length() + 1);
  NormalizeSearchString(buffer.data(), wp->name);
  RadixTree<WaypointPtr>::Remove(buffer.data(), wp);

  if (!wp->shortname.empty()) {
    buffer.GrowDiscard(wp->shortname.length() + 1);
    NormalizeSearchString(buffer.data(), wp->shortname);
    RadixTree<WaypointPtr>::Remove(buffer.data(), wp);
  }
}

Waypoints::Waypoints() noexcept = default;

void
Waypoints::Optimise() noexcept
{
  if (waypoint_tree.IsEmpty() || waypoint_tree.HaveBounds())
    /* empty or already optimised */
    return;

  task_projection.Update();

  for (auto &i : waypoint_tree) {
    // TODO: eliminate this const_cast hack
    Waypoint &w = const_cast<Waypoint &>(*i);
    w.Project(task_projection);
  }

  waypoint_tree.Optimise();
}

void
Waypoints::Append(WaypointPtr wp) noexcept
{
  // TODO: eliminate this const_cast hack
  Waypoint &w = const_cast<Waypoint &>(*wp);

  if (waypoint_tree.HaveBounds()) {
    w.Project(task_projection);
    if (!waypoint_tree.IsWithinBounds(wp))
      ScheduleOptimise();
  } else if (IsEmpty())
    task_projection.Reset(w.location);

  w.flags.watched = w.origin == WaypointOrigin::WATCHED;

  task_projection.Scan(w.location);
  w.id = next_id++;

  waypoint_tree.Add(wp);
  name_tree.Add(wp);

  ++serial;
}

WaypointPtr
Waypoints::GetNearest(const GeoPoint &loc, double range) const noexcept
{
  if (IsEmpty())
    return nullptr;

  const FlatGeoPoint flat_location = task_projection.ProjectInteger(loc);
  const WaypointTree::Point point(flat_location.x, flat_location.y);
  const unsigned mrange = task_projection.ProjectRangeInteger(loc, range);
  const auto found = waypoint_tree.FindNearest(point, mrange);

  if (found.first == waypoint_tree.end())
    return nullptr;

  return *found.first;
}

static constexpr bool
IsLandable(const Waypoint &wp) noexcept
{
  return wp.IsLandable();
}

WaypointPtr
Waypoints::GetNearestLandable(const GeoPoint &loc, double range) const noexcept
{
  return GetNearestIf(loc, range, IsLandable);
}

WaypointPtr
Waypoints::GetNearestIf(const GeoPoint &loc, double range,
                        bool (*predicate)(const Waypoint &)) const noexcept
{
  if (IsEmpty())
    return nullptr;

  const FlatGeoPoint flat_location = task_projection.ProjectInteger(loc);
  const WaypointTree::Point point(flat_location.x, flat_location.y);
  const unsigned mrange = task_projection.ProjectRangeInteger(loc, range);
  const auto found = waypoint_tree.FindNearestIf(point, mrange,
                                                 [predicate](const WaypointPtr &ptr){
                                                   return predicate(*ptr);
                                                 });

  if (found.first == waypoint_tree.end())
    return nullptr;

  return *found.first;
}

WaypointPtr
Waypoints::LookupName(tstring_view name) const noexcept
{
  return name_tree.Get(name);
}

WaypointPtr
Waypoints::LookupLocation(const GeoPoint &loc,
                          const double range) const noexcept
{
  auto wp = GetNearest(loc, range);
  if (!wp)
    return nullptr;

  if (wp->location == loc)
    return wp;
  else if (range > 0 && wp->IsCloseTo(loc, range))
    return wp;

  return nullptr;
}

WaypointPtr
Waypoints::FindHome() noexcept
{
  for (const auto &wp : waypoint_tree) {
    if (wp->flags.home) {
      home = wp;
      return wp;
    }
  }

  return nullptr;
}

bool
Waypoints::SetHome(const unsigned id) noexcept
{
  home = LookupId(id);
  if (home == nullptr)
    return false;

  Waypoint &wp = const_cast<Waypoint &>(*home);
  wp.flags.home = true;
  return true;
}

WaypointPtr
Waypoints::LookupId(const unsigned id) const noexcept
{
  for (const auto &wp : waypoint_tree)
    if (wp->id == id)
      return wp;

  return nullptr;
}

void
Waypoints::VisitWithinRange(const GeoPoint &loc, const double range,
                            WaypointVisitor visitor) const
{
  if (IsEmpty())
    return; // nothing to do

  const FlatGeoPoint flat_location = task_projection.ProjectInteger(loc);
  const WaypointTree::Point point(flat_location.x, flat_location.y);
  const unsigned mrange = task_projection.ProjectRangeInteger(loc, range);

  waypoint_tree.VisitWithinRange(point, mrange, visitor);
}

void
Waypoints::VisitNamePrefix(tstring_view prefix,
                           WaypointVisitor visitor) const
{
  name_tree.VisitNormalisedPrefix(prefix, visitor);
}

void
Waypoints::Clear() noexcept
{
  ++serial;
  home = nullptr;
  name_tree.Clear();
  waypoint_tree.clear();
  next_id = 1;
}

void
Waypoints::Erase(WaypointPtr &&wp) noexcept
{
  if (home == wp)
    home = nullptr;

  auto f = waypoint_tree.FindNearestIf(waypoint_tree.GetPosition(wp), 0,
                                       [&wp](const WaypointPtr &ptr){
                                         return ptr == wp;
                                       });
  assert(f.first != waypoint_tree.end());

  name_tree.Remove(std::move(wp));
  waypoint_tree.erase(f.first);
  ++serial;
}

void
Waypoints::EraseUserMarkers() noexcept
{
  waypoint_tree.EraseIf([this](const WaypointPtr &wp){
      if (wp->origin == WaypointOrigin::USER &&
          wp->type == Waypoint::Type::MARKER) {
        if (home == wp)
          home = nullptr;

        name_tree.Remove(wp);
        ++serial;
        return true;
      } else
        return false;
    });
}

void
Waypoints::Replace(const WaypointPtr &orig, Waypoint &&replacement) noexcept
{
  assert(!waypoint_tree.IsEmpty());

  name_tree.Remove(orig);

  replacement.id = orig->id;

  if (waypoint_tree.HaveBounds()) {
    replacement.Project(task_projection);

    const WaypointTree::Point point(replacement.flat_location.x,
                                    replacement.flat_location.y);
    if (!waypoint_tree.IsWithinBounds(point))
      ScheduleOptimise();
  }

  WaypointPtr new_ptr(new Waypoint(std::move(replacement)));
  name_tree.Add(new_ptr);

  auto f = waypoint_tree.FindNearestIf(waypoint_tree.GetPosition(orig), 0,
                                       [&orig](const WaypointPtr &ptr){
                                         return ptr == orig;
                                       });
  assert(f.first != waypoint_tree.end());

  waypoint_tree.Replace(f.first, std::move(new_ptr));

  ++serial;
}

Waypoint
Waypoints::Create(const GeoPoint &location) noexcept
{
  Waypoint edit_waypoint(location);

  // first waypoint, put into primary file (this will be auto-generated)
  edit_waypoint.origin = WaypointOrigin::USER;
  edit_waypoint.original_id = 0;
  return edit_waypoint;
}

WaypointPtr
Waypoints::CheckExistsOrAppend(WaypointPtr waypoint) noexcept
{
  auto found = LookupName(waypoint->name);
  if (found && found->IsCloseTo(waypoint->location, 100))
    return found;

  Append(waypoint);
  return waypoint;
}

Waypoint
Waypoints::GenerateTakeoffPoint(const GeoPoint& location,
                                const double terrain_alt) const noexcept
{
  // fallback: create a takeoff point
  Waypoint to_point(location);
  to_point.elevation = terrain_alt;
  to_point.has_elevation = true;
  to_point.name = _T("(takeoff)");
  to_point.type = Waypoint::Type::OUTLANDING;
  return to_point;
}

void
Waypoints::AddTakeoffPoint(const GeoPoint& location,
                           const double terrain_alt) noexcept
{
  // remove old one first
  WaypointPtr old_takeoff_point = LookupName(_T("(takeoff)"));
  if (old_takeoff_point != nullptr)
    Erase(std::move(old_takeoff_point));

  if (!GetNearestLandable(location, 5000)) {
    // now add new and update database
    Waypoint new_waypoint = GenerateTakeoffPoint(location, terrain_alt);
    Append(std::move(new_waypoint));
  }

  Optimise();
}
