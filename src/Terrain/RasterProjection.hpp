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

#ifndef XCSOAR_TERRAIN_RASTER_PROJECTION_HPP
#define XCSOAR_TERRAIN_RASTER_PROJECTION_HPP

#include "Navigation/GeoPoint.hpp"
#include "Math/fixed.hpp"
#include "Compiler.h"

#include <utility>

struct GeoBounds;

/**
 * This class manages the projection from GeoPoint to RasterMap
 * coordinates.
 */
class RasterProjection {
  int left, top;
  fixed x_scale, y_scale;

public:
  void set(const GeoBounds &bounds, unsigned width, unsigned height);

  gcc_pure
  Angle width_to_angle(fixed pixels) const {
    return Angle::native(fixed(pixels) / x_scale);
  }

  gcc_pure
  Angle height_to_angle(fixed pixels) const {
    return Angle::native(fixed(pixels) / y_scale);
  }

  gcc_pure std::pair<unsigned, unsigned>
  project(const GeoPoint &location) const {
    unsigned x = (int)(location.Longitude.value_native() * x_scale) - left;
    unsigned y = top - (int)(location.Latitude.value_native() * y_scale);

    return std::pair<unsigned, unsigned>(x, y);
  }

  /**
   * Determines the distance (in meters) of two raster pixels.
   *
   * @param pixels the pixel distance between two pixels
   */
  gcc_pure fixed
  pixel_distance(const GeoPoint &location, unsigned pixels) const;
};

#endif
