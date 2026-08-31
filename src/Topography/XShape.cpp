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
#include "Math/Line2D.hpp"
#endif

#include "Geo/GeoClip.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>
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

  if (count >= 2) {
    if (n_out < max_lines)
      out_counts[n_out++] = count;
    else
      out_pts.resize(out_pts.size() - count);
  }
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
 * Squared distance from #p to the segment #a–#b.
 */
[[gnu::pure]]
static ShapeScalar
PointSegmentDistance2(ShapePoint p, ShapePoint a, ShapePoint b) noexcept
{
  const Line2D<ShapePoint> line(a, b);
  if (line.GetSquaredDistance() == 0)
    return (p - a).MagnitudeSquared();

  double t = line.ProjectedRatio(p);
  if (t < 0)
    t = 0;
  else if (t > 1)
    t = 1;
  return (p - line.Interpolate(t)).MagnitudeSquared();
}

/**
 * Douglas–Peucker keep-flags for src[0..n).  First and last are kept.
 */
static void
SimplifyRingRDP(const ShapePoint *src, unsigned n, ShapeScalar eps2,
                std::vector<char> &keep) noexcept
{
  keep.assign(n, 0);
  if (n < 2)
    return;

  keep.front() = 1;
  keep.back() = 1;

  std::vector<std::pair<unsigned, unsigned>> stack;
  stack.emplace_back(0, n - 1);

  while (!stack.empty()) {
    const auto [first, last] = stack.back();
    stack.pop_back();

    unsigned best = 0;
    ShapeScalar best_d2 = eps2;
    for (unsigned i = first + 1; i < last; ++i) {
      const ShapeScalar d2 =
        PointSegmentDistance2(src[i], src[first], src[last]);
      if (d2 > best_d2) {
        best_d2 = d2;
        best = i;
      }
    }

    if (best != 0) {
      keep[best] = 1;
      stack.emplace_back(first, best);
      stack.emplace_back(best, last);
    }
  }
}

/**
 * Ear-clip a ring.  Rings larger than TARGET are simplified with
 * Douglas–Peucker first: PolygonToTriangles() is O(n²), and a
 * uniform stride used to drop bays so landcover triangles stretched
 * across the map.
 */
static unsigned
PolygonToTrianglesThinned(const ShapePoint *src, unsigned n,
                          GLushort *triangles,
                          ShapeScalar min_distance) noexcept
{
  constexpr unsigned TARGET = 128;
  assert(n > 0 && n - 1 <= 0xffff);

  if (n >= 2 && src[0] == src[n - 1])
    n--;

  if (n <= TARGET)
    return PolygonToTriangles(src, n, triangles, min_distance);

  ShapeScalar min_x = src[0].x, max_x = src[0].x;
  ShapeScalar min_y = src[0].y, max_y = src[0].y;
  for (unsigned i = 1; i < n; ++i) {
    min_x = std::min(min_x, src[i].x);
    max_x = std::max(max_x, src[i].x);
    min_y = std::min(min_y, src[i].y);
    max_y = std::max(max_y, src[i].y);
  }

  const ShapeScalar span = std::max(max_x - min_x, max_y - min_y);
  if (span <= 0)
    return PolygonToTriangles(src, n, triangles, min_distance);

  ShapeScalar eps = span / ShapeScalar(TARGET);
  ShapeScalar eps2 = eps * eps;

  std::vector<char> keep;
  unsigned n_keep = n;
  for (unsigned pass = 0; pass < 8; ++pass) {
    SimplifyRingRDP(src, n, eps2, keep);
    n_keep = 0;
    for (char k : keep)
      n_keep += k != 0;
    if (n_keep <= TARGET)
      break;
    eps2 *= 4;
  }

  if (n_keep < 3)
    return 0;

  std::vector<ShapePoint> thin_pts;
  std::vector<GLushort> orig;
  thin_pts.reserve(n_keep);
  orig.reserve(n_keep);
  for (unsigned i = 0; i < n; ++i) {
    if (keep[i] == 0)
      continue;
    orig.push_back(GLushort(i));
    thin_pts.push_back(src[i]);
  }

  std::vector<GLushort> tmp(3 * (thin_pts.size() - 2));
  const unsigned count =
    PolygonToTriangles(thin_pts.data(), thin_pts.size(), tmp.data(), 0);
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
    /* Strip conversion may restart once per triangle; keep room for
       2 extra indices per restart. */
    const unsigned max_triangles =
      num_points >= 2 * num_lines ? num_points - 2 * num_lines : 0;
    const unsigned max_strip =
      max_triangles == 0 ? 0 : 5 * max_triangles - 2;
    index_count[thinning_level] =
      std::make_unique<GLushort[]>(1 + max_strip);
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
