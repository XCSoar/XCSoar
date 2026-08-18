// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/XShape.hpp"
#include "Convert.hpp"
#include "util/Compiler.h"
#include "util/StaticArray.hxx"
#include "util/StringAPI.hxx"
#include "util/UTF8.hpp"
#include "util/StringStrip.hxx"
#include "util/ScopeExit.hxx"

#ifdef ENABLE_OPENGL
#include "Projection/Projection.hpp"
#include "ui/canvas/opengl/Triangulate.hpp"
#endif

#include "Geo/GeoClip.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>

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
static bool
ShapeFullyInsideClip(const shapeObj &shape, const GeoBounds &clip) noexcept
{
  return clip.IsInside(ImportRect(shape.bounds));
}

static constexpr uint16_t MAX_LINE_POINTS = 16384;

static_assert(XShape::MAX_LINES <= 255,
              "num_lines is stored in a uint8_t");

/**
 * Clip one shapefile line into #out_pts / #out_counts.  A long way
 * that leaves and re-enters the viewport becomes several short
 * polylines.
 */
static void
ClipShapeLine(const lineObj &line, const GeoClip &clip,
              std::vector<GeoPoint> &out_pts,
              std::array<uint16_t, XShape::MAX_LINES> &out_counts,
              uint8_t &n_out, uint8_t max_lines) noexcept
{
  if (line.numpoints < 2 || n_out >= max_lines)
    return;

  uint16_t count = 0;
  for (int j = 1; j < line.numpoints; ++j) {
    GeoPoint a = ToGeoPoint(line.point[j - 1]);
    GeoPoint b = ToGeoPoint(line.point[j]);
    const GeoPoint b_orig = b;
    if (!clip.ClipLine(a, b))
      continue;

    if (count == 0) {
      if (n_out >= max_lines)
        return;
      out_pts.push_back(a);
      out_pts.push_back(b);
      count = 2;
    } else if (count >= MAX_LINE_POINTS) {
      out_counts[n_out++] = count;
      count = 0;
      if (n_out >= max_lines)
        return;
      out_pts.push_back(a);
      out_pts.push_back(b);
      count = 2;
    } else {
      out_pts.push_back(b);
      ++count;
    }

    if (b.longitude != b_orig.longitude ||
        b.latitude != b_orig.latitude) {
      out_counts[n_out++] = count;
      count = 0;
    }
  }

  if (count >= 2 && n_out < max_lines)
    out_counts[n_out++] = count;
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

[[gnu::pure]]
static auto
ImportGeoPoint(const GeoPoint &vertex,
               [[maybe_unused]] const GeoPoint &file_center) noexcept
{
#ifdef ENABLE_OPENGL
  const GeoPoint relative = vertex - file_center;
  return ShapePoint{
    ShapeScalar(relative.longitude.Native()),
    ShapeScalar(relative.latitude.Native()),
  };
#else
  return vertex;
#endif
}

XShape::XShape(const shapeObj &shape, const GeoPoint &file_center,
               const char *_label, const GeoBounds *clip,
               bool *clipped)
  :label(ImportLabel(_label))
{
  if (clipped != nullptr)
    *clipped = false;

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

  const bool do_clip = clip != nullptr &&
    shape.type == MS_SHAPE_LINE &&
    !ShapeFullyInsideClip(shape, *clip);

  if (do_clip) {
    if (clipped != nullptr)
      *clipped = true;

    std::vector<GeoPoint> clipped_pts;
    std::array<uint16_t, XShape::MAX_LINES> clipped_counts{};
    uint8_t n_clipped = 0;
    const GeoClip geo_clip{*clip};

    const std::size_t input_lines = std::min((std::size_t)shape.numlines,
                                             lines.size());
    for (std::size_t l = 0; l < input_lines; ++l)
      ClipShapeLine(shape.line[l], geo_clip,
                    clipped_pts, clipped_counts, n_clipped,
                    uint8_t(lines.size()));

    num_lines = n_clipped;
    if (num_lines == 0)
      return;

    GeoBounds new_bounds = GeoBounds::Invalid();
    for (const GeoPoint &gp : clipped_pts) {
      if (!new_bounds.IsValid())
        new_bounds = GeoBounds(gp);
      else
        new_bounds.Extend(gp);
    }
    if (new_bounds.Check())
      bounds = new_bounds;

    std::size_t n = 0;
    for (uint8_t l = 0; l < num_lines; ++l) {
      lines[l] = clipped_counts[l];
      n += lines[l];
    }
    assert(n == clipped_pts.size());

    points = std::make_unique<Point[]>(n);
    auto *p = points.get();
    for (const GeoPoint &gp : clipped_pts)
      *p++ = ImportGeoPoint(gp, file_center);
    return;
  }

  const std::size_t input_lines = std::min((std::size_t)shape.numlines,
                                           lines.size());
  std::size_t num_points = 0;
  for (std::size_t l = 0; l < input_lines; ++l) {
    if (shape.line[l].numpoints < min_points)
      /* malformed shape */
      continue;

    lines[num_lines] = std::min(shape.line[l].numpoints,
                                int(MAX_LINE_POINTS));
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

/**
 * Ear-clip a ring.  Rings larger than TARGET are subsampled in O(n)
 * first at every thinning level; PolygonToTriangles() itself is O(n²)
 * and a large landcover ring would freeze the topography loader.
 */
static unsigned
PolygonToTrianglesThinned(const ShapePoint *src, unsigned n,
                          GLushort *triangles,
                          ShapeScalar min_distance) noexcept
{
  constexpr unsigned TARGET = 128;
  /* orig[] stores source vertex indices in GLushort. */
  assert(n > 0 && n - 1 <= 0xffff);

  if (n <= TARGET)
    return PolygonToTriangles(src, n, triangles, min_distance);

  StaticArray<ShapePoint, TARGET + 1> thin_pts;
  StaticArray<GLushort, TARGET + 1> orig;

  const unsigned stride = (n + TARGET - 1) / TARGET;
  for (unsigned i = 0; i < n; i += stride) {
    orig.append(i);
    thin_pts.append(src[i]);
  }
  if (orig.back() != GLushort(n - 1)) {
    orig.append(n - 1);
    thin_pts.append(src[n - 1]);
  }

  /* PolygonToTriangles writes at most 3*(m-2) indices for m vertices;
     m <= TARGET+1. */
  GLushort tmp[3 * (TARGET + 1)];
  const unsigned count =
    PolygonToTriangles(thin_pts.data(), thin_pts.size(), tmp, 0);
  for (unsigned j = 0; j < count; ++j)
    triangles[j] = orig[tmp[j]];
  return count;
}

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
      std::size_t count = PolygonToTrianglesThinned(pt, lines[i],
                                                    idx + *idx_count,
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
