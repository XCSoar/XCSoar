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
#include "Airspace.hpp"
#include "AbstractAirspace.hpp"

void 
Airspace::destroy()
{
  delete pimpl_airspace;
}

Airspace::Airspace(AbstractAirspace& airspace,
                   const TaskProjection& tp):
  FlatBoundingBox(airspace.GetBoundingBox(tp)),
  pimpl_airspace(&airspace)
{
  pimpl_airspace->SetTaskProjection(tp);
}


bool 
Airspace::inside(const AircraftState &loc) const
{
  if (pimpl_airspace) {
    return pimpl_airspace->Inside(loc);
  } else {
    return false;
  }
}


bool 
Airspace::inside(const GeoPoint &loc) const
{
  if (pimpl_airspace) {
    return pimpl_airspace->Inside(loc);
  } else {
    return false;
  }
}


bool 
Airspace::intersects(const FlatRay& ray) const
{
  return FlatBoundingBox::Intersects(ray);
}


AirspaceIntersectionVector
Airspace::intersects(const GeoPoint& g1,
                     const GeoVector &vec) const
{
  if (pimpl_airspace) {
    return pimpl_airspace->Intersects(g1, vec);
  } else {
    AirspaceIntersectionVector null;
    return null;
  }
}

void 
Airspace::set_ground_level(const fixed alt) const
{
  if (pimpl_airspace) 
    pimpl_airspace->SetGroundLevel(alt);
  else
    assert(1);
}

void 
Airspace::set_flight_level(const AtmosphericPressure &press) const
{
  if (pimpl_airspace) 
    pimpl_airspace->SetFlightLevel(press);
  else
    assert(1);
}

void
Airspace::set_activity(const AirspaceActivity mask) const
{
  if (pimpl_airspace)
    pimpl_airspace->SetActivity(mask);
  else
    assert(1);
}

void
Airspace::clear_clearance() const
{
  if (pimpl_airspace)
    pimpl_airspace->ClearClearance();
  else
    assert(1);
}
