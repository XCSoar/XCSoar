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

#ifndef TOPOGRAPHY_XSHAPE_HPP
#define TOPOGRAPHY_XSHAPE_HPP

#include "Util/NonCopyable.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Geo/GeoBounds.hpp"
#include "shapelib/mapserver.h"
#include "shapelib/mapshape.h"
#ifdef ENABLE_OPENGL
#include "Screen/Point.hpp"
#include "Topography/XShapePoint.hpp"
#endif

#include <tchar.h>
#include <assert.h>

class XShape : private NonCopyable {
  enum { MAX_LINES = 32 };
#ifdef ENABLE_OPENGL
  enum { THINNING_LEVELS = 4 };
#endif

  GeoBounds bounds;
#ifdef ENABLE_OPENGL
  GeoPoint center;
#endif

  unsigned char type;

  /**
   * The number of elements in the "lines" array.
   */
  unsigned char num_lines;

  /**
   * An array which stores the number of points of each line.  This is
   * a fixed-size array to reduce the number of allocations at
   * runtime.
   */
  unsigned short lines[MAX_LINES];

  /**
   * All points of all lines.
   */
#ifdef ENABLE_OPENGL
  ShapePoint *points;

  /**
   * Indices of polygon triangles or lines with reduced number of vertices.
   */
  unsigned short *indices[THINNING_LEVELS];

  /**
   * For polygons this will contain the total number of triangle vertices
   * for each thinning level.
   * For lines there will be an array of size num_lines for each thinning
   * level, which contains the number of points for each line.
   */
  unsigned short *index_count[THINNING_LEVELS];
#else // !ENABLE_OPENGL
  GeoPoint *points;
#endif

  TCHAR *label;

public:
  XShape(shapefileObj *shpfile, int i, int label_field=-1);
  ~XShape();

#ifdef ENABLE_OPENGL
protected:
  bool BuildIndices(unsigned thinning_level, unsigned min_distance);

public:
  const unsigned short *get_indices(int thinning_level, unsigned min_distance,
                                    const unsigned short *&count) const;
#endif

  const GeoBounds &get_bounds() const {
    return bounds;
  }

#ifdef ENABLE_OPENGL
  const GeoPoint &get_center() const {
    return center;
  }
#endif

  MS_SHAPE_TYPE get_type() const {
    return (MS_SHAPE_TYPE)type;
  }

  unsigned get_number_of_lines() const {
    return num_lines;
  }

  const unsigned short *get_lines() const {
    return lines;
  }

#ifdef ENABLE_OPENGL
  const ShapePoint *get_points() const {
#else
  const GeoPoint *get_points() const {
#endif
    return points;
  }

  const TCHAR *get_label() const {
    return label;
  }

#ifdef ENABLE_OPENGL
  /**
   * Convert a GeoPoint into a ShapePoint.
   */
  ShapePoint geo_to_shape(const GeoPoint &location) const {
    return geo_to_shape(center, location);
  }

  /**
   * Get the offset of the shape center from the screen center in ShapePoint
   * scale.
   */
  ShapePoint shape_translation(const GeoPoint &screen_center) const {
    return geo_to_shape(screen_center, center);
  }

private:
  gcc_pure
  ShapePoint geo_to_shape(const GeoPoint &origin, const GeoPoint &point) const;
#endif
};

#endif
