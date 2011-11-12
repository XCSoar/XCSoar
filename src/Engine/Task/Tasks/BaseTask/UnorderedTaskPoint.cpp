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

#include "UnorderedTaskPoint.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

UnorderedTaskPoint::UnorderedTaskPoint(const Waypoint &wp,
                                       const TaskBehaviour &tb)
  :TaskWaypoint(UNORDERED, wp),
   safety_height_arrival(tb.safety_height_arrival) {}

void
UnorderedTaskPoint::SetTaskBehaviour(const TaskBehaviour &tb)
{
  safety_height_arrival = tb.safety_height_arrival;
}

const GeoVector 
UnorderedTaskPoint::GetVectorRemaining(const GeoPoint &reference) const
{
  return GeoVector(reference, GetLocationRemaining());
}

// These are dummies, never get called usually

const GeoVector 
UnorderedTaskPoint::GetVectorPlanned() const
{
  return GeoVector(fixed_zero);
}

const GeoVector 
UnorderedTaskPoint::GetVectorTravelled() const
{
  return GeoVector(fixed_zero);
}

const AircraftState& 
UnorderedTaskPoint::GetEnteredState() const 
{
  // this should never get called
  static const AircraftState null_state;
  return null_state;
}


fixed 
UnorderedTaskPoint::GetElevation() const
{
  return GetBaseElevation() + safety_height_arrival;
}
