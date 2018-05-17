/*
Copyright_License {

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

#include "RasterProjection.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/FAISphere.hpp"

#include <algorithm>
#include <assert.h>

void
RasterProjection::Set(const GeoBounds &bounds,
                      unsigned width, unsigned height)
{
  x_scale = double(width) / bounds.GetWidth().Native();
  left = AngleToWidth(bounds.GetWest());

  y_scale = double(height) / bounds.GetHeight().Native();
  top = AngleToHeight(bounds.GetNorth());
}

double
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

  // must have called Set() first otherwise this is invalid
  assert(x_scale != 0);
  assert(y_scale != 0);

  Angle distance = WidthToAngle(M_SQRT2 * FACTOR * pixels);
  GeoPoint p = GeoPoint(location.longitude + distance, location.latitude);
  auto x = location.DistanceS(p);

  distance = HeightToAngle(M_SQRT2 * FACTOR * pixels);
  p = GeoPoint(location.longitude, location.latitude + distance);
  auto y = location.DistanceS(p);

  return std::max(x, y) / FACTOR;
}

unsigned
RasterProjection::DistancePixelsFine(double distance) const
{
  Angle angle = Angle::Radians(distance / FAISphere::REARTH);
  return AngleToHeight(angle);
}
