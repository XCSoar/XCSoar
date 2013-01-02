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

#include "Airspace.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceIntersectionVector.hpp"
#include "Geo/Flat/TaskProjection.hpp"

void 
Airspace::Destroy()
{
  delete airspace;
}

Airspace::Airspace(AbstractAirspace& airspace,
                   const TaskProjection& tp):
  FlatBoundingBox(airspace.GetBoundingBox(tp)),
  airspace(&airspace)
{
}

Airspace::Airspace(const GeoPoint &loc, const TaskProjection &task_projection,
                   const fixed range)
  :FlatBoundingBox(task_projection.ProjectInteger(loc),
                   task_projection.ProjectRangeInteger(loc, range)),
   airspace(nullptr)
{
}

Airspace::Airspace(const GeoPoint &ll, const GeoPoint &ur,
                   const TaskProjection &task_projection)
  :FlatBoundingBox(task_projection.ProjectInteger(ll),
                   task_projection.ProjectInteger(ur)),
   airspace(nullptr)
{
}

bool 
Airspace::IsInside(const AircraftState &loc) const
{
  if (airspace) {
    return airspace->Inside(loc);
  } else {
    return false;
  }
}


bool 
Airspace::IsInside(const GeoPoint &loc) const
{
  if (airspace) {
    return airspace->Inside(loc);
  } else {
    return false;
  }
}


bool 
Airspace::Intersects(const FlatRay& ray) const
{
  return FlatBoundingBox::Intersects(ray);
}


AirspaceIntersectionVector
Airspace::Intersects(const GeoPoint& g1, const GeoPoint &end,
                     const TaskProjection &projection) const
{
  if (airspace) {
    return airspace->Intersects(g1, end, projection);
  } else {
    AirspaceIntersectionVector null;
    return null;
  }
}

void 
Airspace::SetGroundLevel(const fixed alt) const
{
  if (airspace) 
    airspace->SetGroundLevel(alt);
  else
    assert(1);
}

bool
Airspace::NeedGroundLevel() const
{
  if (airspace)
    return airspace->NeedGroundLevel();
  else
    return false;
}

void 
Airspace::SetFlightLevel(const AtmosphericPressure &press) const
{
  if (airspace) 
    airspace->SetFlightLevel(press);
  else
    assert(1);
}

void
Airspace::SetActivity(const AirspaceActivity mask) const
{
  if (airspace)
    airspace->SetActivity(mask);
  else
    assert(1);
}

void
Airspace::ClearClearance() const
{
  if (airspace)
    airspace->ClearClearance();
  else
    assert(1);
}
