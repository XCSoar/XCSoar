/*
Copyright_License {

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

#include "RasterProjection.hpp"
#include "Geo/GeoBounds.hpp"
#include "Math/Earth.hpp"

void
RasterProjection::set(const GeoBounds &bounds,
                      unsigned width, unsigned height)
{
  x_scale = fixed(width) /
    (bounds.east - bounds.west).AsBearing().Native();
  left = bounds.west.Native() * x_scale;

  y_scale = fixed(height) /
    (bounds.north - bounds.south).AsBearing().Native();
  top = bounds.north.Native() * y_scale;
}

fixed
RasterProjection::pixel_distance(const GeoPoint &location, unsigned pixels) const
{
  enum {
    /**
     * This factor is used to reduce fixed point rounding errors.
     * x_scale and y_scale are quite large numbers, and building their
     * reciprocals may lose a lot of precision.
     */
    FACTOR = 256,
  };

  Angle distance = width_to_angle(fixed_sqrt_two * FACTOR * pixels);
  GeoPoint p = GeoPoint(location.longitude + distance, location.latitude);
  fixed x = location.Distance(p);

  distance = height_to_angle(fixed_sqrt_two * FACTOR * pixels);
  p = GeoPoint(location.longitude, location.latitude + distance);
  fixed y = location.Distance(p);

  return max(x, y) / FACTOR;
}

unsigned
RasterProjection::distance_pixels(fixed distance) const
{
  Angle angle = Angle::Radians(distance / REARTH);
  return angle_to_height(angle);
}
