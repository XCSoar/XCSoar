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

#include "WaypointDialogs.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/LastUsed.hpp"

/**
 * Opens up the WaypointDetails window of the nearest
 * waypoint to location
 * @param way_points Waypoints including all possible
 * waypoints for the calculation
 * @param location Location where to search
 * @param range Maximum range to search
 * @param pan True if in Pan mode
 * @return True if a waypoint was found
 */
bool
PopupNearestWaypointDetails(const Waypoints &way_points,
                            const GeoPoint &location,
                            double range)
{
  auto way_point = way_points.LookupLocation(location, range);

  if (way_point) {
    LastUsedWaypoints::Add(*way_point);
    dlgWaypointDetailsShowModal(way_point, true, true);
    return true;
  }
  return false;
}

