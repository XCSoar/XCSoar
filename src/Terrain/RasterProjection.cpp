/*
Copyright_License {

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

#include "RasterProjection.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/Constants.hpp"

void
RasterProjection::Set(const GeoBounds &bounds,
                      unsigned width, unsigned height)
{
  x_scale = fixed(width) / bounds.GetWidth().Native();
  left = int(bounds.GetWest().Native() * x_scale);

  y_scale = fixed(height) / bounds.GetHeight().Native();
  top = int(bounds.GetNorth().Native() * y_scale);
}

fixed
RasterProjection::FinePixelDistance(const GeoPoint &location,
                                    unsigned pixels) const
{
  enum {
    /**
     * This factor is used to reduce fixed point rounding errors.
     * x_scale and y_scale are quite large numbers, and building their
     * reciprocals may lose a lot of precision.
     */
    FACTOR = 256,
  };

  Angle distance = WidthToAngle(fixed_sqrt_two * FACTOR * pixels);
  GeoPoint p = GeoPoint(location.longitude + distance, location.latitude);
  fixed x = location.Distance(p);

  distance = HeightToAngle(fixed_sqrt_two * FACTOR * pixels);
  p = GeoPoint(location.longitude, location.latitude + distance);
  fixed y = location.Distance(p);

  return std::max(x, y) / FACTOR;
}

unsigned
RasterProjection::DistancePixelsFine(fixed distance) const
{
  Angle angle = Angle::Radians(distance / REARTH);
  return AngleToHeight(angle);
}
