/* Copyright_License {

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

#include "AnnularSectorZone.hpp"
#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

OZBoundary
AnnularSectorZone::GetBoundary() const
{
  OZBoundary boundary;

  const unsigned steps = 20;
  const Angle delta = Angle::FullCircle() / steps;
  const Angle start = GetStartRadial().AsBearing();
  Angle end = GetEndRadial().AsBearing();
  if (end <= start + Angle::FullCircle() / 512)
    end += Angle::FullCircle();

  const GeoPoint inner_start =
    GeoVector(GetInnerRadius(), GetStartRadial()).EndPoint(GetReference());
  const GeoPoint inner_end =
    GeoVector(GetInnerRadius(), GetEndRadial()).EndPoint(GetReference());

  GeoVector inner_vector(GetInnerRadius(), start + delta);
  for (; inner_vector.bearing < end; inner_vector.bearing += delta)
    boundary.push_front(inner_vector.EndPoint(GetReference()));

  boundary.push_front(inner_end);
  boundary.push_front(inner_start);

  GeoVector vector(GetRadius(), start + delta);
  for (; vector.bearing < end; vector.bearing += delta)
    boundary.push_front(vector.EndPoint(GetReference()));

  boundary.push_front(GetSectorEnd());
  boundary.push_front(GetSectorStart());

  return boundary;
}

bool
AnnularSectorZone::IsInSector(const GeoPoint &location) const
{
  GeoVector f(GetReference(), location);

  return (f.distance <= GetRadius()) &&
    (f.distance >= inner_radius) &&
    IsAngleInSector(f.bearing);
}

bool
AnnularSectorZone::Equals(const ObservationZonePoint &other) const
{
  const AnnularSectorZone &z = (const AnnularSectorZone &)other;

  return SectorZone::Equals(other) && inner_radius == z.inner_radius;
}
