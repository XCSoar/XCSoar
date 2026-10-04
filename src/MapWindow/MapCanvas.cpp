// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapCanvas.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "Math/Screen.hpp"
#include "Geo/SearchPointVector.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Triangulate.hpp"
#include "ui/opengl/System.hpp"
#endif

void
MapCanvas::DrawLine(GeoPoint a, GeoPoint b) noexcept
{
  if (!clip.ClipLine(a, b))
    return;

  canvas.DrawLine(projection.GeoToScreen(a), projection.GeoToScreen(b));
}

void
MapCanvas::DrawLineWithOffset(GeoPoint a, GeoPoint b) noexcept
{
  if (!clip.ClipLine(a, b))
    return;

  const auto p_a = projection.GeoToScreen(a);
  const auto p_b = projection.GeoToScreen(b);
  const auto p_end = ScreenClosestPoint(p_a, p_b, p_a, Layout::Scale(20));
  canvas.DrawLine(p_b, p_end);
}


void
MapCanvas::DrawCircle(const GeoPoint &center, double radius) noexcept
{
  auto screen_center = projection.GeoToScreen(center);
  unsigned screen_radius = projection.GeoToScreenDistance(radius);
  canvas.DrawCircle(screen_center, screen_radius);
}

void
MapCanvas::Project(const Projection &projection,
                   const SearchPointVector &points, BulkPixelPoint *screen) noexcept
{
  for (const auto i : points)
    *screen++ = projection.GeoToScreen(i.GetLocation());
}

#ifdef ENABLE_OPENGL
/**
 * Ear-clip a geographic ring in a local equirectangular frame.
 * Drops a closing duplicate.  @p n is updated to the vertex count
 * used.  Indices refer to src[0..n).
 *
 * @return triangle index count, or 0 on failure
 */
static unsigned
TriangulateGeoRing(const GeoPoint *src, unsigned &n,
                   AllocatedArray<uint16_t> &indices) noexcept
{
  if (n >= 2 && src[0] == src[n - 1])
    n--;
  if (n < 3)
    return 0;

  AllocatedArray<FloatPoint2D> flat;
  flat.GrowDiscard(n);
  const GeoPoint origin = src[0];
  const double cos_lat = origin.latitude.cos();
  for (unsigned i = 0; i < n; ++i) {
    const double x =
      (src[i].longitude - origin.longitude).Native() * cos_lat;
    const double y = (src[i].latitude - origin.latitude).Native();
    flat[i] = FloatPoint2D(float(x), float(y));
  }

  indices.GrowDiscard(3 * (n - 2));
  return PolygonToTriangles(flat.data(), n,
                            reinterpret_cast<GLushort *>(indices.data()),
                            0.f);
}

bool
MapCanvas::BuildTriangleCache(const SearchPointVector &points,
                              AllocatedArray<uint16_t> &indices,
                              unsigned &index_count,
                              unsigned &vertex_count) noexcept
{
  index_count = 0;
  vertex_count = 0;
  const unsigned n_src = points.size();
  if (n_src < 3 || n_src > 65535)
    return false;

  AllocatedArray<GeoPoint> geo;
  geo.GrowDiscard(n_src);
  for (unsigned i = 0; i < n_src; ++i)
    geo[i] = points[i].GetLocation();

  unsigned n = n_src;
  index_count = TriangulateGeoRing(geo.data(), n, indices);
  vertex_count = n;
  return index_count >= 3;
}

bool
MapCanvas::PreparePolygon(const SearchPointVector &points,
                          const uint16_t *indices, unsigned index_count,
                          unsigned vertex_count) noexcept
{
  prepared_src = nullptr;
  prepared_indices = nullptr;
  num_triangle_indices = 0;
  screen_ready = false;

  if (indices == nullptr || index_count < 3 || vertex_count < 3)
    return false;

  GeoBounds bb(points[0].GetLocation());
  for (unsigned i = 1; i < vertex_count; ++i)
    bb.Extend(points[i].GetLocation());
  if (bb.IsValid() && !clip.Overlaps(bb))
    return false;

  prepared_src = &points;
  prepared_indices = indices;
  num_triangle_indices = index_count;
  num_raster_points = vertex_count;
  return true;
}

bool
MapCanvas::PreparePolygon(const SearchPointVector &points,
                          bool skip_triangulate) noexcept
{
  const unsigned n = points.size();
  if (n < 3)
    return false;

  geo_points.GrowDiscard(n * 4);
  for (unsigned i = 0; i < n; ++i)
    geo_points[i] = points[i].GetLocation();
  return PrepareCopied(n, skip_triangulate);
}
#endif

bool
MapCanvas::PreparePolygon(const SearchPointVector &points) noexcept
{
#ifdef ENABLE_OPENGL
  return PreparePolygon(points, false);
#else
  const unsigned n = points.size();
  if (n < 3)
    return false;

  geo_points.GrowDiscard(n * 4);
  for (unsigned i = 0; i < n; ++i)
    geo_points[i] = points[i].GetLocation();
  return PrepareCopied(n);
#endif
}

bool
MapCanvas::PreparePolygon(const GeoPoint *src, unsigned num_points) noexcept
{
  if (num_points < 3)
    return false;

  geo_points.GrowDiscard(num_points * 4);
  for (unsigned i = 0; i < num_points; ++i)
    geo_points[i] = src[i];
  return PrepareCopied(num_points);
}

bool
MapCanvas::PrepareCopied(unsigned num_points,
                         bool skip_triangulate) noexcept
{
  GeoBounds bb(geo_points[0]);
  for (unsigned i = 1; i < num_points; ++i)
    bb.Extend(geo_points[i]);
  if (bb.IsValid() && !clip.Overlaps(bb))
    return false;

#ifdef ENABLE_OPENGL
  prepared_src = nullptr;
  prepared_indices = nullptr;
  num_triangle_indices = 0;
  screen_ready = false;
  if (!skip_triangulate) {
    unsigned n = num_points;
    num_triangle_indices = TriangulateGeoRing(geo_points.data(), n,
                                              triangle_indices);
    if (num_triangle_indices >= 3) {
      num_raster_points = n;
      prepared_indices = triangle_indices.data();
      return true;
    }
    num_triangle_indices = 0;
  }
#else
  (void)skip_triangulate;
#endif

  num_raster_points = clip.ClipPolygon(geo_points.data(),
                                       geo_points.data(), num_points);
  if (num_raster_points < 3)
    return false;

  raster_points.GrowDiscard(num_raster_points);
  for (unsigned i = 0; i < num_raster_points; ++i)
    raster_points[i] = projection.GeoToScreen(geo_points[i]);

  return true;
}

void
MapCanvas::DrawPrepared() noexcept
{
#ifdef ENABLE_OPENGL
  if (num_triangle_indices >= 3 && prepared_indices != nullptr) {
    if (!screen_ready) {
      screen_points.GrowDiscard(num_raster_points);
      if (prepared_src != nullptr) {
        for (unsigned i = 0; i < num_raster_points; ++i) {
          const auto sp =
            projection.GeoToScreen((*prepared_src)[i].GetLocation());
          screen_points[i] = FloatPoint2D(float(sp.x), float(sp.y));
        }
      } else {
        for (unsigned i = 0; i < num_raster_points; ++i) {
          const auto sp = projection.GeoToScreen(geo_points[i]);
          screen_points[i] = FloatPoint2D(float(sp.x), float(sp.y));
        }
      }
      screen_ready = true;
    }
    canvas.DrawFilledTriangles(screen_points.data(),
                               reinterpret_cast<const GLushort *>(
                                 prepared_indices),
                               num_triangle_indices);
    return;
  }
#endif

  canvas.DrawPolygon(raster_points.data(), num_raster_points);
}

static void
FlushOutlineStrip(Canvas &canvas, const BulkPixelPoint *strip,
                  unsigned &length) noexcept
{
  if (length >= 2)
    canvas.DrawPolyline(strip, length);
  length = 0;
}

template<typename GetPoint>
static void
DrawPolygonOutlineN(Canvas &canvas, const Projection &projection,
                    const GeoClip &clip, unsigned n,
                    GetPoint get) noexcept
{
  if (n < 2)
    return;

  if (get(0) == get(n - 1))
    n--;
  if (n < 2)
    return;

  AllocatedArray<BulkPixelPoint> strip;
  strip.GrowDiscard(n + 1);
  unsigned strip_len = 0;

  for (unsigned i = 0; i < n; ++i) {
    GeoPoint a = get(i);
    GeoPoint b = get((i + 1) % n);
    if (!clip.ClipLine(a, b)) {
      FlushOutlineStrip(canvas, strip.data(), strip_len);
      continue;
    }

    const BulkPixelPoint sa(projection.GeoToScreen(a));
    const BulkPixelPoint sb(projection.GeoToScreen(b));
    if (strip_len > 0 && strip[strip_len - 1] != sa)
      FlushOutlineStrip(canvas, strip.data(), strip_len);

    if (strip_len == 0)
      strip[strip_len++] = sa;
    strip[strip_len++] = sb;
  }

  FlushOutlineStrip(canvas, strip.data(), strip_len);
}

void
MapCanvas::DrawPolygonOutline(const SearchPointVector &points) noexcept
{
  DrawPolygonOutlineN(canvas, projection, clip, points.size(),
                      [&](unsigned i) {
                        return points[i].GetLocation();
                      });
}

void
MapCanvas::DrawPolygonOutline(const GeoPoint *src,
                              unsigned num_points) noexcept
{
  DrawPolygonOutlineN(canvas, projection, clip, num_points,
                      [&](unsigned i) { return src[i]; });
}
