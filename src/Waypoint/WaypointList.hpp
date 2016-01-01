/*
Copyright_License {

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

#ifndef XCSOAR_WAYPOINT_LIST_HPP
#define XCSOAR_WAYPOINT_LIST_HPP

#include "Geo/GeoVector.hpp"
#include "Engine/Waypoint/Ptr.hpp"

#include <vector>

/**
 * Structure to hold Waypoint sorting information
 */
struct WaypointListItem
{
  WaypointPtr waypoint;

private:
  /** From observer to waypoint */
  mutable GeoVector vec = GeoVector::Invalid();

public:
  template<typename W>
  explicit WaypointListItem(W &&_waypoint):
    waypoint(std::forward<W>(_waypoint)) {}

  void ResetVector();
  const GeoVector &GetVector(const GeoPoint &location) const;
};

class WaypointList: public std::vector<WaypointListItem>
{
public:
  void SortByDistance(const GeoPoint &location);
};

#endif
