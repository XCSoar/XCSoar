// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
#include <span>

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

#ifdef ENABLE_OPENGL
  using Point = ShapePoint;
#else
  using Point = GeoPoint;
#endif

  /**
   * All points of all lines.
   */
  std::unique_ptr<Point[]> points;

#ifdef ENABLE_OPENGL
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
#endif

  BasicAllocatedString<char> label;

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

  std::span<const uint16_t> GetLines() const noexcept {
    return { lines.data(), num_lines };
  }

  const Point *GetPoints() const noexcept {
    return points.get();
  }

  const char *GetLabel() const noexcept {
    return label.c_str();
  }
};
