/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "VariableKeyholeZone.hpp"
#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

//------------------------------------------------------------------------------
OZBoundary
VariableKeyholeZone::GetBoundary() const
{
  OZBoundary boundary;
  boundary.push_front(this->GetSectorStart());
  boundary.push_front(this->GetSectorEnd());

  boundary.GenerateArcExcluding(this->GetReference(),
                                this->GetRadius(),
                                this->GetStartRadial(),
                                this->GetEndRadial());

  const auto small_radius = this->GetInnerRadius();
  GeoVector small_vector(small_radius, this->GetStartRadial());
  boundary.push_front(small_vector.EndPoint(this->GetReference()));
  small_vector.bearing = this->GetEndRadial();
  boundary.push_front(small_vector.EndPoint(this->GetReference()));

  boundary.GenerateArcExcluding(this->GetReference(),
                                small_radius,
                                this->GetEndRadial(),
                                this->GetStartRadial());

  return boundary;
}

//------------------------------------------------------------------------------
bool
VariableKeyholeZone::IsInSector(const GeoPoint &location) const
{
  GeoVector f(GetReference(), location);

  return ((f.distance <= this->inner_radius) ||
          (f.distance <= this->GetRadius() &&
           this->IsAngleInSector(f.bearing)));
}

