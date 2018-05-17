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

#ifndef TOPOGRAPHY_XSHAPE_HPP
#define TOPOGRAPHY_XSHAPE_HPP

#include "Util/ConstBuffer.hxx"
#include "Util/AllocatedString.hxx"
#include "Geo/GeoBounds.hpp"
#include "shapelib/mapserver.h"
#include "shapelib/mapshape.h"
#ifdef ENABLE_OPENGL
#include "Topography/XShapePoint.hpp"
#endif

#include <tchar.h>
#include <stdint.h>

struct GeoPoint;

class XShape {
  static constexpr unsigned MAX_LINES = 32;
#ifdef ENABLE_OPENGL
  static constexpr unsigned THINNING_LEVELS = 4;
#endif

  GeoBounds bounds;

  uint8_t type;

  /**
   * The number of elements in the "lines" array.
   */
  uint8_t num_lines;

  /**
   * An array which stores the number of points of each line.  This is
   * a fixed-size array to reduce the number of allocations at
   * runtime.
   */
  uint16_t lines[MAX_LINES];

  /**
   * All points of all lines.
   */
#ifdef ENABLE_OPENGL
  ShapePoint *points;

  /**
   * Indices of polygon triangles or lines with reduced number of vertices.
   */
  uint16_t *indices[THINNING_LEVELS];

  /**
   * For polygons this will contain the total number of triangle vertices
   * for each thinning level.
   * For lines there will be an array of size num_lines for each thinning
   * level, which contains the number of points for each line.
   */
  uint16_t *index_count[THINNING_LEVELS];

  /**
   * The start offset in the #GLArrayBuffer (vertex buffer object).
   * It is managed by #TopographyFileRenderer.
   */
  mutable unsigned offset;
#else // !ENABLE_OPENGL
  GeoPoint *points;
#endif

  AllocatedString<TCHAR> label;

public:
  XShape(shapefileObj *shpfile, const GeoPoint &file_center, int i,
         int label_field=-1);

  XShape(const XShape &) = delete;

  ~XShape();

#ifdef ENABLE_OPENGL
  void SetOffset(unsigned _offset) const {
    offset = _offset;
  }

  unsigned GetOffset() const {
    return offset;
  }

protected:
  bool BuildIndices(unsigned thinning_level, ShapeScalar min_distance);

public:
  const uint16_t *GetIndices(int thinning_level,
                             ShapeScalar min_distance,
                             const uint16_t *&count) const;
#endif

  const GeoBounds &get_bounds() const {
    return bounds;
  }

  MS_SHAPE_TYPE get_type() const {
    return (MS_SHAPE_TYPE)type;
  }

  ConstBuffer<uint16_t> GetLines() const {
    return { lines, num_lines };
  }

#ifdef ENABLE_OPENGL
  const ShapePoint *GetPoints() const {
#else
  const GeoPoint *GetPoints() const {
#endif
    return points;
  }

  const TCHAR *GetLabel() const {
    return label.c_str();
  }
};

#endif
