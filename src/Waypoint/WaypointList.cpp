// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointList.hpp"
#include "Waypoint/Waypoint.hpp"

#include <algorithm>

void
WaypointListItem::ResetVector() noexcept
{
  vec.SetInvalid();
}

const GeoVector &
WaypointListItem::GetVector(const GeoPoint &location) const noexcept
{
  if (!vec.IsValid())
    vec = GeoVector(location, waypoint->location);

  return vec;
}

void
WaypointList::SortByDistance(const GeoPoint &location) noexcept
{
  std::sort(begin(), end(), [location](const auto &a, const auto &b){
    return a.GetVector(location).distance < b.GetVector(location).distance;
  });
}

void
WaypointList::SortByName() noexcept
{
  std::sort(begin(), end(), [](const auto &a, const auto &b){
    return a.waypoint->name < b.waypoint->name;
  });
}

void
WaypointList::MakeUnique() noexcept
{
  auto new_end = std::unique(begin(), end(), [](const auto &a, const auto &b){
    return a.waypoint == b.waypoint;
  });

  erase(new_end, end());
}
