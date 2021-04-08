/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

class WaypointDistanceCompare
{
  const GeoPoint &location;

public:
  explicit WaypointDistanceCompare(const GeoPoint &_location) noexcept
    :location(_location) {}


  [[gnu::pure]]
  bool operator()(const WaypointListItem &a,
                  const WaypointListItem &b) const noexcept {
    return a.GetVector(location).distance < b.GetVector(location).distance;
  }
};

void
WaypointList::SortByDistance(const GeoPoint &location) noexcept
{
  std::sort(begin(), end(), WaypointDistanceCompare(location));
}
