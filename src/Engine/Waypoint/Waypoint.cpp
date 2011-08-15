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

#include "Waypoint.hpp"
#include "Navigation/TaskProjection.hpp"

void
Waypoint::Flags::SetDefaultFlags(bool turnpoint)
{
  turn_point = turnpoint;
  home = false;
  start_point = false;
  finish_point = false;
  watched = false;
}

Waypoint::Waypoint(const GeoPoint &_location, const bool is_turnpoint):
  location(_location),
#ifndef NDEBUG
  flat_location_initialised(false),
#endif
  type(wtNormal)
{
  flags.SetDefaultFlags(is_turnpoint);
  runway.Clear();
  radio_frequency.Clear();
}

bool
Waypoint::IsCloseTo(const GeoPoint &_location, const fixed range) const
{
  return location.distance(_location) <= range;
}

void
Waypoint::Project(const TaskProjection &task_projection)
{
  flat_location = task_projection.project(location);

#ifndef NDEBUG
  flat_location_initialised = true;
#endif
}
