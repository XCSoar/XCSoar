/* Copyright_License {

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

#include "Airspace.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceIntersectionVector.hpp"

Airspace::Airspace(AirspacePtr _airspace,
                   const FlatProjection &tp) noexcept
  :FlatBoundingBox(_airspace->GetBoundingBox(tp)),
   airspace(std::move(_airspace))
{
}

bool
Airspace::IsInside(const AircraftState &loc) const noexcept
{
  assert(airspace != nullptr);
  return airspace->Inside(loc);
}


bool
Airspace::IsInside(const GeoPoint &loc) const noexcept
{
  assert(airspace != nullptr);
  return airspace->Inside(loc);
}

AirspaceIntersectionVector
Airspace::Intersects(const GeoPoint &g1, const GeoPoint &end,
                     const FlatProjection &projection) const noexcept
{
  assert(airspace != nullptr);
  return airspace->Intersects(g1, end, projection);
}

void
Airspace::SetGroundLevel(const double alt) const noexcept
{
  assert(airspace != nullptr);
  airspace->SetGroundLevel(alt);
}

bool
Airspace::NeedGroundLevel() const noexcept
{
  assert(airspace != nullptr);
  return airspace->NeedGroundLevel();
}

void
Airspace::SetFlightLevel(const AtmosphericPressure &press) const noexcept
{
  assert(airspace != nullptr);
  airspace->SetFlightLevel(press);
}

void
Airspace::SetActivity(const AirspaceActivity mask) const noexcept
{
  assert(airspace != nullptr);
  airspace->SetActivity(mask);
}

void
Airspace::ClearClearance() const noexcept
{
  assert(airspace != nullptr);
  airspace->ClearClearance();
}
