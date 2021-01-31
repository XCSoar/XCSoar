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

#include "GeoPoint.hpp"
#include "GeoVector.hpp"
#include "Math.hpp"
#include "SimplifiedMath.hpp"

GeoPoint 
GeoPoint::Parametric(const GeoPoint &delta, const double t) const
{
  return (*this) + delta * t;
}

GeoPoint 
GeoPoint::Interpolate(const GeoPoint &end, const double t) const
{
  return (*this) + (end - (*this)) * t;
}

double
GeoPoint::Distance(const GeoPoint &other) const
{
  return ::Distance(*this, other);
}

Angle
GeoPoint::Bearing(const GeoPoint &other) const
{
  return ::Bearing(*this, other);
}

GeoVector
GeoPoint::DistanceBearing(const GeoPoint &other) const
{
  GeoVector gv;
  ::DistanceBearing(*this, other, &gv.distance, &gv.bearing);
  return gv;
}

double
GeoPoint::DistanceS(const GeoPoint &other) const
{
  double distance;
  ::DistanceBearingS(*this, other, &distance, nullptr);
  return distance;
}

Angle
GeoPoint::BearingS(const GeoPoint &other) const
{
  Angle angle;
  ::DistanceBearingS(*this, other, (Angle *)nullptr, &angle);
  return angle;
}

GeoVector
GeoPoint::DistanceBearingS(const GeoPoint &other) const
{
  GeoVector gv;
  ::DistanceBearingS(*this, other, &gv.distance, &gv.bearing);
  return gv;
}

double 
GeoPoint::ProjectedDistance(const GeoPoint &from,
                             const GeoPoint &to) const
{
  return ::ProjectedDistance(from, to, *this);
}

GeoPoint
GeoPoint::Middle(const GeoPoint &other) const
{
  return ::Middle(*this, other);
}

bool 
GeoPoint::Sort(const GeoPoint &sp) const
{
  if (longitude < sp.longitude)
    return false;
  else if (longitude == sp.longitude)
    return latitude > sp.latitude;
  else
    return true;
}

GeoPoint 
GeoPoint::IntermediatePoint(const GeoPoint &destination, 
                            const double distance) const
{
  return ::IntermediatePoint(*this, destination, distance);
}
