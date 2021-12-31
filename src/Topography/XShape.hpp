/*

Copyright_License {

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

#ifndef TOPOGRAPHY_XSHAPE_HPP
#define TOPOGRAPHY_XSHAPE_HPP

#include "util/ConstBuffer.hxx"
#include "util/AllocatedString.hxx"
#include "Geo/GeoBounds.hpp"
#include "shapelib/mapserver.h"
#include "shapelib/mapshape.h"
#ifdef ENABLE_OPENGL
#include "Topography/XShapePoint.hpp"
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <tchar.h>

#include <tchar.h>

struct GeoPoint;

class XShape {
  static constexpr std::size_t MAX_LINES = 32;
#ifdef ENABLE_OPENGL
  static constexpr std::size_t THINNING_LEVELS = 4;
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
  std::array<uint16_t, MAX_LINES> lines;

  /**
   * All points of all lines.
   */
#ifdef ENABLE_OPENGL
  std::unique_ptr<ShapePoint[]> points;

  /**
   * Indices of polygon triangles or lines with reduced number of vertices.
   */
  std::array<uint16_t *, THINNING_LEVELS> indices{};

  /**
   * For polygons this will contain the total number of triangle vertices
   * for each thinning level.
   * For lines there will be an array of size num_lines for each thinning
   * level, which contains the number of points for each line.
   */
  std::array<std::unique_ptr<uint16_t[]>, THINNING_LEVELS> index_count;

  /**
   * The start offset in the #GLArrayBuffer (vertex buffer object).
   * It is managed by #TopographyFileRenderer.
   */
  mutable unsigned offset;
#else // !ENABLE_OPENGL
  std::unique_ptr<GeoPoint[]> points;
#endif

  BasicAllocatedString<TCHAR> label;

public:
  /**
   * Throws on error.
   */
  XShape(const shapeObj &shape, const GeoPoint &file_center,
         const char *label);

  ~XShape() noexcept;

  XShape(const XShape &) = delete;
  XShape &operator=(const XShape &) = delete;

#ifdef ENABLE_OPENGL
  void SetOffset(unsigned _offset) const noexcept {
    offset = _offset;
  }

  unsigned GetOffset() const noexcept {
    return offset;
  }

protected:
  bool BuildIndices(unsigned thinning_level,
                    ShapeScalar min_distance) noexcept;

public:
  struct Indices {
    const uint16_t *indices;
    const uint16_t *count;
  };

  [[gnu::pure]]
  Indices GetIndices(int thinning_level,
                     ShapeScalar min_distance) const noexcept;
#endif

  const GeoBounds &get_bounds() const noexcept {
    return bounds;
  }

  MS_SHAPE_TYPE get_type() const noexcept {
    return (MS_SHAPE_TYPE)type;
  }

  ConstBuffer<uint16_t> GetLines() const noexcept {
    return { lines.data(), num_lines };
  }

#ifdef ENABLE_OPENGL
  const ShapePoint *GetPoints() const noexcept {
#else
  const GeoPoint *GetPoints() const noexcept {
#endif
    return points.get();
  }

  const TCHAR *GetLabel() const noexcept {
    return label.c_str();
  }
};

#endif
