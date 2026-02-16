// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/XShape.hpp"
#include "Convert.hpp"
#include "util/Compiler.h"
#include "util/StringAPI.hxx"
#include "util/UTF8.hpp"
#include "util/StringStrip.hxx"
#include "util/ScopeExit.hxx"

#ifdef ENABLE_OPENGL
#include "Projection/Projection.hpp"
#include "ui/canvas/opengl/Triangulate.hpp"
#endif

#include <algorithm>
#include <stdexcept>

#include <tchar.h>

static BasicAllocatedString<char>
ImportLabel(const char *src) noexcept
{
  if (src == nullptr)
    return nullptr;

  src = StripLeft(src);
  if (StringIsEqual(src, "RAILWAY STATION") ||
      StringIsEqual(src, "RAILROAD STATION") ||
      StringIsEqual(src, "UNK"))
    return nullptr;

  if (!ValidateUTF8(src))
    return nullptr;

  return BasicAllocatedString<char>(src);
}

/**
 * Returns the minimum number of points for each line of this shape
 * type.  Returns -1 if the shape type is not supported.
 */
static constexpr int
GetMinPointsForShapeType(int shapelib_type) noexcept
{
  switch (shapelib_type) {
  case MS_SHAPE_POINT:
    return 1;

  case MS_SHAPE_LINE:
    return 2;

  case MS_SHAPE_POLYGON:
    return 3;

  default:
    /* not supported */
    return -1;
  }
}

static constexpr GeoPoint
ToGeoPoint(const pointObj &src) noexcept
{
  return {
    Angle::Degrees(src.x),
    Angle::Degrees(src.y),
  };
}

[[gnu::pure]]
static auto
ImportShapePoint(const pointObj &src, [[maybe_unused]] const GeoPoint &file_center) noexcept
{
#ifdef ENABLE_OPENGL
  /* OpenGL: convert GeoPoints to ShapePoints, make them relative to
     the map's boundary center */

  const GeoPoint vertex = ToGeoPoint(src);
  const GeoPoint relative = vertex - file_center;

  return ShapePoint{
    ShapeScalar(relative.longitude.Native()),
    ShapeScalar(relative.latitude.Native()),
  };
#else
  /* convert all points of all lines to GeoPoints */
  return ToGeoPoint(src);
#endif
}

XShape::XShape(const shapeObj &shape, const GeoPoint &file_center,
               const char *_label)
  :label(ImportLabel(_label))
{
  bounds = ImportRect(shape.bounds);
  if (!bounds.Check())
    throw std::runtime_error{"Malformed shape bounds"};

  type = shape.type;

  num_lines = 0;

  const int min_points = GetMinPointsForShapeType(shape.type);
  if (min_points < 0) {
    /* not supported, leave an empty XShape object */
    return;
  }

  const std::size_t input_lines = std::min((std::size_t)shape.numlines,
                                           lines.size());
  std::size_t num_points = 0;
  for (std::size_t l = 0; l < input_lines; ++l) {
    if (shape.line[l].numpoints < min_points)
      /* malformed shape */
      continue;

    lines[num_lines] = std::min(shape.line[l].numpoints, 16384);
    num_points += lines[num_lines];
    ++num_lines;
  }

  points = std::make_unique<Point[]>(num_points);
  auto *p = points.get();
  for (std::size_t l = 0; l < num_lines; ++l) {
    const pointObj *src = shape.line[l].point;
    p = std::transform(src, src + lines[l], p,
                       [&](const auto &src){
                         return ImportShapePoint(src, file_center);
                       });
  }
}

XShape::~XShape() noexcept = default;

#ifdef ENABLE_OPENGL

inline bool
XShape::BuildIndices(unsigned thinning_level, ShapeScalar min_distance) noexcept
{
  assert(indices[thinning_level] == nullptr);

  uint16_t *idx, *idx_count;
  std::size_t num_points = 0;

  for (std::size_t i=0; i < num_lines; i++)
    num_points += lines[i];

  if (type == MS_SHAPE_LINE) {
    if (num_points <= 2)
      return false;  // line cannot be simplified, so don't create indices
    index_count[thinning_level] = std::make_unique<GLushort[]>(num_lines + num_points);
    idx_count = index_count[thinning_level].get();
    indices[thinning_level] = idx = idx_count + num_lines;

    const auto end_l = std::next(lines.begin(), num_lines);
    const ShapePoint *p = points.get();
    unsigned i = 0;
    for (auto l = lines.begin(); l != end_l; ++l) {
      assert(*l >= 2);
      const ShapePoint *end_p = p + *l - 1;
      // always add first point
      *idx++ = i;
      p++; i++;
      const uint16_t *after_first_idx = idx;
      // add points if they are not too close to the previous point
      for (; p < end_p; p++, i++)
        if (ManhattanDistance(points[idx[-1]], *p) >= min_distance)
          *idx++ = i;
      // remove points from behind if they are too close to the end point
      while (idx > after_first_idx &&
             ManhattanDistance(points[idx[-1]], *p) < min_distance)
        idx--;
      // always add last point
      *idx++ = i;
      p++; i++;
      *idx_count++ = idx - after_first_idx + 1;
    }
    // TODO: free memory saved by thinning (use malloc/realloc or some class?)
    return true;
  } else if (type == MS_SHAPE_POLYGON) {
    index_count[thinning_level] = std::make_unique<GLushort[]>(1 + 3 * (num_points - 2) + 2 * (num_lines - 1));
    idx_count = index_count[thinning_level].get();
    indices[thinning_level] = idx = idx_count + 1;

    *idx_count = 0;
    const ShapePoint *pt = points.get();
    for (std::size_t i=0; i < num_lines; i++) {
      std::size_t count = PolygonToTriangles(pt, lines[i], idx + *idx_count,
                                             min_distance);
      if (i > 0) {
        const GLushort offset = pt - points.get();
        const std::size_t max_idx_count = *idx_count + count;
        for (std::size_t j = *idx_count; j < max_idx_count; j++)
          idx[j] += offset;
      }
      *idx_count += count;
      pt += lines[i];
    }
    *idx_count = TriangleToStrip(idx, *idx_count, num_points, num_lines);
    // TODO: free memory saved by thinning (use malloc/realloc or some class?)
    return true;
  } else {
    gcc_unreachable();
  }
}

XShape::Indices
XShape::GetIndices(int thinning_level, ShapeScalar min_distance) const noexcept
{
  if (indices[thinning_level] == nullptr) {
    XShape &deconst = const_cast<XShape &>(*this);
    if (!deconst.BuildIndices(thinning_level, min_distance))
      return {};
  }

  return {indices[thinning_level], index_count[thinning_level].get()};
}

#endif // ENABLE_OPENGL
