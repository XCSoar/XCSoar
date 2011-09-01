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

#include "GeoVector.hpp"
#include "Math/Earth.hpp"

GeoVector::GeoVector(const GeoPoint &source, const GeoPoint &target)
{
  *this = source.distance_bearing(target);
}

GeoPoint
GeoVector::end_point(const GeoPoint &source) const
{
  if (!positive(Distance))
    return source;

  return ::FindLatitudeLongitude(source, Bearing, Distance);
}

GeoPoint
GeoVector::mid_point(const GeoPoint &source) const
{
  if (!positive(Distance))
    return source;

  return ::FindLatitudeLongitude(source, Bearing, half(Distance));
}

fixed
GeoVector::minimum_distance(const GeoPoint &source,
                            const GeoPoint &ref) const
{
  return ::CrossTrackError(source, end_point(source), ref, NULL);
}

GeoPoint 
GeoVector::intermediate_point(const GeoPoint &source, 
                              const fixed distance) const
{
  return source.intermediate_point(end_point(source), distance);
}
