/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_TERRAIN_RASTER_PROJECTION_HPP
#define XCSOAR_TERRAIN_RASTER_PROJECTION_HPP

#include "Terrain/RasterLocation.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/fixed.hpp"
#include "Compiler.h"

struct GeoBounds;

/**
 * This class manages the projection from GeoPoint to RasterMap
 * coordinates.
 */
class RasterProjection {
  int left, top;
  fixed x_scale, y_scale;

public:
  void Set(const GeoBounds &bounds, unsigned width, unsigned height);

  gcc_pure
  Angle WidthToAngle(fixed pixels) const {
    return Angle::Native(fixed(pixels) / x_scale);
  }

  gcc_pure
  Angle HeightToAngle(fixed pixels) const {
    return Angle::Native(fixed(pixels) / y_scale);
  }

  gcc_pure
  unsigned AngleToHeight(Angle angle) const {
    return (unsigned)(angle.Native() * y_scale);
  }

  gcc_pure RasterLocation
  ProjectFine(const GeoPoint &location) const {
    const unsigned x = ((int)(location.longitude.Native() * x_scale)) - left;
    const unsigned y = top - ((int)(location.latitude.Native() * y_scale));

    return RasterLocation(x, y);
  }

  gcc_pure
  GeoPoint
  UnprojectFine(const RasterLocation &coords) const {
    const fixed x = fixed((int)coords.x + left) / x_scale;
    const fixed y = fixed(top - (int)coords.y) / y_scale;
    return GeoPoint(Angle::Native(x),Angle::Native(y));
  }

  gcc_pure RasterLocation
  ProjectCoarse(const GeoPoint &location) const {
    const int x = ((int)(location.longitude.Native() * x_scale)) - left;
    const int y = top - ((int)(location.latitude.Native() * y_scale));

    return RasterLocation(x >> 8, y >> 8);
  }

  gcc_pure
  GeoPoint
  UnprojectCoarse(const RasterLocation &coords) const {
    const fixed x = fixed(((int)coords.x << 8) + left) / x_scale;
    const fixed y = fixed(top - ((int)coords.y << 8)) / y_scale;
    return GeoPoint(Angle::Native(x),Angle::Native(y));
  }

  /**
   * Determines the distance (in meters) of two raster pixels.
   *
   * @param pixels the pixel distance between two pixels
   */
  gcc_pure fixed
  PixelDistance(const GeoPoint &location, unsigned pixels) const;

  /**
   * Converts a distance (in meters) to a pixel distance.
   *
   * @param pixels the pixel distance between two pixels
   */
  gcc_pure unsigned
  DistancePixels(fixed distance) const;
};

#endif
