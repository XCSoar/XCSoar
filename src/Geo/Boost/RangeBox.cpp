/*
Copyright_License {

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

#include "RangeBox.hpp"
#include "Geo/FAISphere.hpp"

boost::geometry::model::box<GeoPoint>
BoostRangeBox(const GeoPoint location, double range)
{
  Angle latitude_delta = FAISphere::EarthDistanceToAngle(range);

  Angle north = std::min(location.latitude + latitude_delta,
                         Angle::QuarterCircle());
  Angle south = std::max(location.latitude - latitude_delta,
                         -Angle::QuarterCircle());

  auto c = std::max(location.latitude.cos(), 0.01);
  Angle longitude_delta = std::min(latitude_delta / c, Angle::QuarterCircle());

  Angle west = (location.longitude - longitude_delta).AsDelta();
  Angle east = (location.longitude + longitude_delta).AsDelta();

  return {GeoPoint(west, south), GeoPoint(east, north)};
}
