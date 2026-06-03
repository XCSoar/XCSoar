// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/ForwardViewTopographyOverlay.hpp"
#include "ForwardView/ForwardViewGeometry.hpp"
#include "ForwardView/ForwardViewTerrain.hpp"
#include "Look/Colors.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "Projection/WindowProjection.hpp"
#include "Terrain/Height.hpp"
#include "Terrain/RasterMap.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoVector.hpp"
#include "ui/canvas/Color.hpp"
#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Triangulate.hpp"
#endif

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace ForwardViewTopography {

static GeoBounds
ForwardViewGeoBounds(const GeoPoint &origin, Angle track,
                     double range, float aspect) noexcept
{
  const double near_half =
    ForwardViewGeometry::LateralHalfWidthAtDistance(500., aspect);
  const double far_half =
    ForwardViewGeometry::LateralHalfWidthAtDistance(range, aspect);
  const Angle lateral = track + Angle::QuarterCircle();
  const GeoPoint near_left =
    GeoVector{near_half, lateral + Angle::HalfCircle()}.EndPoint(origin);
  const GeoPoint near_right =
    GeoVector{near_half, lateral}.EndPoint(origin);
  const GeoPoint far_center = GeoVector{range, track}.EndPoint(origin);
  const GeoPoint far_left =
    GeoVector{far_half, lateral + Angle::HalfCircle()}.EndPoint(far_center);
  const GeoPoint far_right =
    GeoVector{far_half, lateral}.EndPoint(far_center);

  GeoBounds bounds(near_left);
  bounds.Extend(near_right);
  bounds.Extend(far_left);
  bounds.Extend(far_right);
  return bounds.Scale(1.2);
}

/** Meters per pixel for forward-view topology visibility (matches frustum width). */
double
ForwardViewMapScale(const PixelRect &rc, double range, float aspect) noexcept
{
  const unsigned dim = std::max(rc.GetWidth(), rc.GetHeight());
  if (dim == 0)
    return 1.;

  const double width =
    2. * ForwardViewGeometry::LateralHalfWidthAtDistance(range, aspect);
  return width / double(dim);
}

WindowProjection
CorridorProjection(GeoPoint start, Angle track, double range, float aspect,
                   const PixelRect &rc) noexcept
{
  const double far_half =
    ForwardViewGeometry::LateralHalfWidthAtDistance(range, aspect);

  WindowProjection projection;
  projection.SetScreenSize(rc.GetSize());
  const GeoPoint center =
    GeoVector{range * 0.5, track}.EndPoint(start);
  projection.SetGeoLocation(center);
  projection.SetScreenOrigin(rc.GetCenter());
  projection.SetScreenAngle(track);
  projection.SetScaleFromRadius(std::max(range * 0.5, far_half));
  projection.UpdateScreenBounds();
  return projection;
}

static bool
GeoToLocal(const GeoPoint &origin, Angle track, const GeoPoint &point,
           double &x, double &y) noexcept
{
  if (!point.IsValid())
    return false;

  const GeoVector rel(origin, point);
  const Angle rel_bearing = rel.bearing - track;
  x = rel.distance * rel_bearing.cos();
  y = rel.distance * rel_bearing.sin();
  return true;
}

static bool
InViewCorridor(double x, double y, double range, float aspect) noexcept
{
  constexpr double margin = 500.;
  if (x < -margin || x > range + margin)
    return false;

  const double half_w =
    ForwardViewGeometry::LateralHalfWidthAtDistance(std::max(x, 50.), aspect)
    + margin;
  return y >= -half_w && y <= half_w;
}

static double
DisplayZ(const RasterMap &map, GeoPoint point,
         double x, double y,
         double aircraft_alt, double vertical_ref_alt) noexcept
{
  const TerrainHeight h = ForwardViewTerrain::HeightAt(map, point);
  if (h.IsInvalid())
    return 0.;

  return ForwardViewTerrain::MeshElevation(h.GetValue(), aircraft_alt,
                                           vertical_ref_alt, x, y, h) +
         ForwardViewGeometry::TOPO_SURFACE_OFFSET;
}

static Vertex
MakeVertex(double x, double y, double z, Color color) noexcept
{
  return {
    float(x), float(y), float(z),
    float(color.Red()) / 255.f,
    float(color.Green()) / 255.f,
    float(color.Blue()) / 255.f,
    float(color.Alpha()) / 255.f,
  };
}

static void
AppendLine(std::vector<Vertex> &vertices,
           double x1, double y1, double z1,
           double x2, double y2, double z2,
           Color color) noexcept
{
  vertices.push_back(MakeVertex(x1, y1, z1, color));
  vertices.push_back(MakeVertex(x2, y2, z2, color));
}

static bool
LocalPointAt(const GeoPoint &origin, Angle track, const GeoPoint &geo,
             double range, float aspect,
             double &x, double &y) noexcept
{
  if (!GeoToLocal(origin, track, geo, x, y))
    return false;

  return InViewCorridor(x, y, range, aspect);
}

static bool
SegmentSlopeOk(double x1, double y1, double z1,
               double x2, double y2, double z2) noexcept
{
  static constexpr double MAX_GRADE = 1.5;

  const double horiz = std::hypot(x2 - x1, y2 - y1);
  if (horiz < 0.01)
    return true;

  return std::abs(z2 - z1) / horiz <= MAX_GRADE;
}

static bool
SegmentVisibleFromEye(const RasterMap &map, const GeoPoint &origin,
                        Angle track, double aircraft_alt,
                        double vertical_ref_alt,
                        double x1, double y1, double z1,
                        double x2, double y2, double z2) noexcept
{
  if (!ForwardViewTerrain::PointVisibleFromEye(map, origin, track,
                                               aircraft_alt, vertical_ref_alt,
                                               x1, y1, z1))
    return false;

  if (!ForwardViewTerrain::PointVisibleFromEye(map, origin, track,
                                               aircraft_alt, vertical_ref_alt,
                                               x2, y2, z2))
    return false;

  const double mx = (x1 + x2) * 0.5;
  const double my = (y1 + y2) * 0.5;
  const double mz = (z1 + z2) * 0.5;
  return ForwardViewTerrain::PointVisibleFromEye(map, origin, track,
                                                 aircraft_alt, vertical_ref_alt,
                                                 mx, my, mz);
}

static void
AppendDrapedSegment(std::vector<Vertex> &vertices,
                    const GeoPoint &origin, Angle track, double range,
                    float aspect, double aircraft_alt, double vertical_ref_alt,
                    const RasterMap &map, Color color,
                    GeoPoint from, GeoPoint to) noexcept
{
  static constexpr double STEP = 100.;

  const GeoVector seg(from, to);
  const unsigned n_steps =
    std::max(1u, unsigned(std::ceil(seg.distance / STEP)));

  GeoPoint prev = from;
  double prev_x = 0., prev_y = 0., prev_z = 0.;
  bool have_prev = LocalPointAt(origin, track, prev, range, aspect,
                                prev_x, prev_y);
  if (have_prev)
    prev_z = DisplayZ(map, prev, prev_x, prev_y,
                      aircraft_alt, vertical_ref_alt);

  for (unsigned step = 1; step <= n_steps; ++step) {
    const GeoPoint geo =
      GeoVector{seg.distance * double(step) / double(n_steps),
                seg.bearing}.EndPoint(from);
    double x = 0., y = 0.;
    if (!LocalPointAt(origin, track, geo, range, aspect, x, y)) {
      have_prev = false;
      prev = geo;
      continue;
    }

    const double z = DisplayZ(map, geo, x, y,
                             aircraft_alt, vertical_ref_alt);
    if (have_prev &&
        SegmentSlopeOk(prev_x, prev_y, prev_z, x, y, z) &&
        SegmentVisibleFromEye(map, origin, track, aircraft_alt,
                              vertical_ref_alt,
                              prev_x, prev_y, prev_z, x, y, z))
      AppendLine(vertices, prev_x, prev_y, prev_z, x, y, z, color);
    else if (have_prev)
      have_prev = false;

    prev_x = x;
    prev_y = y;
    prev_z = z;
    have_prev = true;
    prev = geo;
  }
}

static void
AppendPolyline(std::vector<Vertex> &vertices,
               const GeoPoint &origin, Angle track, double range,
               float aspect, double aircraft_alt, double vertical_ref_alt,
               const RasterMap &map,
               const TopographyFile &file,
               const ShapePoint *points, unsigned n_points,
               unsigned skip, Color color) noexcept
{
  GeoPoint prev_geo = GeoPoint::Invalid();

  for (unsigned i = 0; i < n_points; i += skip) {
    const GeoPoint geo = file.ToGeoPoint(points[i]);
    if (!prev_geo.IsValid()) {
      prev_geo = geo;
      continue;
    }

    AppendDrapedSegment(vertices, origin, track, range, aspect,
                        aircraft_alt, vertical_ref_alt,
                        map, color, prev_geo, geo);
    prev_geo = geo;
  }
}

#ifdef ENABLE_OPENGL

static bool
SimilarToWaterColor(Color c) noexcept
{
  const Color w = COLOR_FORWARD_VIEW_WATER;
  return std::abs(int(c.Red()) - int(w.Red())) < 48 &&
         std::abs(int(c.Green()) - int(w.Green())) < 48 &&
         std::abs(int(c.Blue()) - int(w.Blue())) < 48;
}

static Color
PolygonFillColor(Color file_color) noexcept
{
  if (SimilarToWaterColor(file_color))
    return COLOR_FORWARD_VIEW_TOPO_WATER.WithAlpha(
      std::max(file_color.Alpha(), uint8_t(0xe0)));

  if (file_color.Alpha() < 0xe0)
    return file_color.WithAlpha(0xe0);

  return file_color;
}

static bool
ProjectPolygonPoint(const GeoPoint &origin, Angle track, const GeoPoint &geo,
                    double &x, double &y) noexcept
{
  return GeoToLocal(origin, track, geo, x, y);
}

static double
PolygonFillZ(const RasterMap &map, GeoPoint geo,
             double x, double y,
             double aircraft_alt, double vertical_ref_alt,
             bool flat_water) noexcept
{
  if (flat_water) {
    static const TerrainHeight water(int16_t(-30000));
    return ForwardViewTerrain::MeshElevation(0., aircraft_alt,
                                             vertical_ref_alt, x, y, water) +
           ForwardViewGeometry::TOPO_FILL_OFFSET;
  }

  const TerrainHeight h = ForwardViewTerrain::HeightAt(map, geo);
  if (h.IsInvalid())
    return ForwardViewGeometry::TOPO_FILL_OFFSET;

  return ForwardViewTerrain::MeshElevation(h.GetValue(), aircraft_alt,
                                           vertical_ref_alt, x, y, h) +
         ForwardViewGeometry::TOPO_FILL_OFFSET;
}

struct ProjectedPoint {
  float x, y, z;
  bool valid = false;
};

static bool
AppendFillTriangle(std::vector<Vertex> &vertices,
                   const ProjectedPoint &a,
                   const ProjectedPoint &b,
                   const ProjectedPoint &c,
                   Color color) noexcept
{
  if (!a.valid || !b.valid || !c.valid)
    return false;

  vertices.push_back(MakeVertex(a.x, a.y, a.z, color));
  vertices.push_back(MakeVertex(b.x, b.y, b.z, color));
  vertices.push_back(MakeVertex(c.x, c.y, c.z, color));
  return true;
}

static void
ProjectPolygonRing(const GeoPoint &origin, Angle track,
                   double aircraft_alt, double vertical_ref_alt,
                   const RasterMap &map, bool flat_water,
                   const TopographyFile &file,
                   const ShapePoint *points, unsigned n_points,
                   std::vector<ProjectedPoint> &projected,
                   unsigned offset) noexcept
{
  for (unsigned i = 0; i < n_points; ++i) {
    const unsigned idx = offset + i;
    if (idx >= projected.size() || projected[idx].valid)
      continue;

    const GeoPoint geo = file.ToGeoPoint(points[i]);
    double x = 0., y = 0.;
    if (!ProjectPolygonPoint(origin, track, geo, x, y))
      continue;

    projected[idx].x = float(x);
    projected[idx].y = float(y);
    projected[idx].z = float(PolygonFillZ(map, geo, x, y,
                                        aircraft_alt, vertical_ref_alt,
                                        flat_water));
    projected[idx].valid = true;
  }
}

static bool
PointInShapeRing(const ShapePoint *ring, unsigned n_points,
                 double px, double py) noexcept
{
  bool inside = false;
  for (unsigned i = 0, j = n_points - 1; i < n_points; j = i++) {
    const double xi = double(ring[i].x), yi = double(ring[i].y);
    const double xj = double(ring[j].x), yj = double(ring[j].y);
    if (((yi > py) != (yj > py)) &&
        (px < (xj - xi) * (py - yi) / (yj - yi) + xi))
      inside = !inside;
  }

  return inside;
}

/** True for outers and nested islands; false for holes (matches msIsOuterRing). */
static bool
IsOuterRing(const ShapePoint *const *rings, const unsigned *ring_sizes,
            unsigned num_rings, unsigned ring_index) noexcept
{
  if (num_rings <= 1)
    return true;

  const unsigned n_points = ring_sizes[ring_index];
  if (n_points < 3)
    return false;

  const ShapePoint *ring = rings[ring_index];

  bool status = true;
  for (unsigned i = 0; i < num_rings; ++i) {
    if (i == ring_index || ring_sizes[i] < 3)
      continue;

    const ShapePoint *other = rings[i];
    const unsigned on = ring_sizes[i];
    const bool in0 = PointInShapeRing(other, on,
                                      double(ring[0].x), double(ring[0].y));
    const bool in1 = PointInShapeRing(other, on,
                                      double(ring[1].x), double(ring[1].y));
    if (in0 == in1) {
      if (in0)
        status = !status;
    } else if (n_points > 2 &&
               PointInShapeRing(other, on,
                                double(ring[2].x), double(ring[2].y)))
      status = !status;
  }

  return status;
}

static unsigned
AppendPolygonRingFill(std::vector<Vertex> &vertices,
                      const ProjectedPoint *ring, unsigned n_points,
                      const ShapePoint *shape_points,
                      ShapeScalar min_distance, Color fill_color) noexcept
{
  if (n_points < 3)
    return 0;

  std::vector<GLushort> tri_indices((n_points - 2) * 3);
  const unsigned n_idx = PolygonToTriangles(
    shape_points, n_points, tri_indices.data(), min_distance);

  unsigned n_triangles = 0;
  if (n_idx >= 3) {
    for (unsigned t = 0; t + 2 < n_idx; t += 3) {
      const unsigned i0 = tri_indices[t];
      const unsigned i1 = tri_indices[t + 1];
      const unsigned i2 = tri_indices[t + 2];
      if (i0 >= n_points || i1 >= n_points || i2 >= n_points)
        continue;

      if (AppendFillTriangle(vertices, ring[i0], ring[i1], ring[i2], fill_color))
        ++n_triangles;
    }
  }

  if (n_triangles > 0)
    return n_triangles;

  for (unsigned i = 1; i + 1 < n_points; ++i) {
    if (AppendFillTriangle(vertices, ring[0], ring[i], ring[i + 1], fill_color))
      ++n_triangles;
  }

  return n_triangles;
}

static void
AppendPolygonFill(std::vector<Vertex> &vertices,
                  const GeoPoint &origin, Angle track,
                  double aircraft_alt, double vertical_ref_alt,
                  const RasterMap &map, Color color,
                  const TopographyFile &file,
                  const XShape &shape,
                  double map_scale) noexcept
{
  const auto lines = shape.GetLines();
  const unsigned n_points =
    std::accumulate(lines.begin(), lines.end(), 0u);
  if (n_points < 3)
    return;

  const Color fill_color = PolygonFillColor(color);
  const bool flat_water = SimilarToWaterColor(color);
  const ShapePoint *points = shape.GetPoints();
  std::vector<ProjectedPoint> projected(n_points);

  unsigned num_rings = 0;
  for (const unsigned n_ring : lines)
    if (n_ring >= 3)
      ++num_rings;

  std::vector<unsigned> ring_sizes;
  std::vector<const ShapePoint *> ring_ptrs;
  ring_sizes.reserve(num_rings);
  ring_ptrs.reserve(num_rings);
  unsigned offset = 0;
  for (const unsigned n_ring : lines) {
    if (n_ring >= 3) {
      ring_sizes.push_back(n_ring);
      ring_ptrs.push_back(points + offset);
    }
    offset += n_ring;
  }

  const unsigned level = file.GetThinningLevel(map_scale);
  const ShapeScalar min_distance =
    ShapeScalar(file.GetMinimumPointDistance(level));

  for (unsigned ring_index = 0; ring_index < num_rings; ++ring_index) {
    if (!IsOuterRing(ring_ptrs.data(), ring_sizes.data(), num_rings,
                     ring_index))
      continue;

    const unsigned n_ring = ring_sizes[ring_index];
    const unsigned ring_offset = unsigned(ring_ptrs[ring_index] - points);
    ProjectPolygonRing(origin, track, aircraft_alt, vertical_ref_alt,
                       map, flat_water, file, ring_ptrs[ring_index], n_ring,
                       projected, ring_offset);
    (void)AppendPolygonRingFill(vertices, projected.data() + ring_offset,
                                n_ring, ring_ptrs[ring_index],
                                min_distance, fill_color);
  }
}

void
BuildWaterFills(std::vector<Vertex> &fill_vertices,
                TopographyStore &store,
                const RasterMap &map,
                GeoPoint start, GeoVector forward,
                double aircraft_alt, double vertical_ref_alt,
                const PixelRect &rc) noexcept
{
  if (!forward.IsValid() || !start.IsValid())
    return;

  const float aspect = float(rc.GetWidth()) / float(std::max(1u, rc.GetHeight()));
  const double range = forward.distance;

  WindowProjection projection =
    CorridorProjection(start, forward.bearing, range, aspect, rc);

  const double map_scale = ForwardViewMapScale(rc, range, aspect);
  store.ScanVisibility(projection, 4, map_scale);
  const GeoBounds view_bounds =
    ForwardViewGeoBounds(start, forward.bearing, range, aspect);
  const Angle track = forward.bearing;

  for (const TopographyFile &file : store) {
    const std::lock_guard lock{file.mutex};

    if (!file.IsVisible(map_scale))
      continue;

    const Color color{file.GetColor()};
    if (!SimilarToWaterColor(color))
      continue;

    for (const XShape &shape : file) {
      if (!view_bounds.Overlaps(shape.get_bounds()))
        continue;

      if (shape.get_type() != MS_SHAPE_POLYGON)
        continue;

      AppendPolygonFill(fill_vertices, start, track,
                        aircraft_alt, vertical_ref_alt,
                        map, color, file, shape, map_scale);
    }
  }
}

#endif

static void
AppendSprites(std::vector<Sprite> &sprites,
              const GeoPoint &origin, Angle track, double range,
              float aspect, double aircraft_alt, double vertical_ref_alt,
              const RasterMap &map,
              const TopographyFile &file,
              const ShapePoint *points, unsigned n_points,
              unsigned skip) noexcept
{
  if (!file.GetIcon().IsDefined())
    return;

  for (unsigned i = 0; i < n_points; i += skip) {
    const GeoPoint geo = file.ToGeoPoint(points[i]);
    double x = 0., y = 0.;
    if (!LocalPointAt(origin, track, geo, range, aspect, x, y))
      continue;

    const double z = DisplayZ(map, geo, x, y,
                             aircraft_alt, vertical_ref_alt);
    if (!ForwardViewTerrain::PointVisibleFromEye(map, origin, track,
                                                 aircraft_alt, vertical_ref_alt,
                                                 x, y, z))
      continue;

    sprites.push_back({
      float(x), float(y), float(z),
      file.GetIcon(), file.GetBigIcon(), file.GetUltraIcon(),
    });
  }
}

void
BuildSprites(std::vector<Sprite> &sprites,
             TopographyStore &store,
             const RasterMap &map,
             GeoPoint start, GeoVector forward,
             double aircraft_alt, double vertical_ref_alt,
             const PixelRect &rc) noexcept
{
  if (!forward.IsValid() || !start.IsValid())
    return;

  const float aspect = float(rc.GetWidth()) / float(std::max(1u, rc.GetHeight()));
  const double range = forward.distance;

  WindowProjection projection =
    CorridorProjection(start, forward.bearing, range, aspect, rc);

  const double map_scale = ForwardViewMapScale(rc, range, aspect);
  store.ScanVisibility(projection, 4, map_scale);
  const GeoBounds view_bounds =
    ForwardViewGeoBounds(start, forward.bearing, range, aspect);
  const Angle track = forward.bearing;

  for (const TopographyFile &file : store) {
    const std::lock_guard lock{file.mutex};

    if (!file.IsVisible(map_scale))
      continue;

    const unsigned skip = file.GetSkipSteps(map_scale);

    for (const XShape &shape : file) {
      if (!view_bounds.Overlaps(shape.get_bounds()))
        continue;

      if (shape.get_type() != MS_SHAPE_POINT)
        continue;

      const auto lines = shape.GetLines();
      const ShapePoint *points = shape.GetPoints();
      unsigned offset = 0;
      for (const unsigned n_points : lines) {
        AppendSprites(sprites, start, track, range, aspect,
                      aircraft_alt, vertical_ref_alt,
                      map, file, points + offset, n_points, skip);
        offset += n_points;
      }
    }
  }
}

void
BuildOverlay(std::vector<Vertex> &line_vertices,
             std::vector<Vertex> &fill_vertices,
             std::vector<Sprite> &sprites,
             TopographyStore &store,
             const RasterMap &map,
             GeoPoint start, GeoVector forward,
             double aircraft_alt, double vertical_ref_alt,
             const PixelRect &rc) noexcept
{
  if (!forward.IsValid() || !start.IsValid())
    return;

  const float aspect = float(rc.GetWidth()) / float(std::max(1u, rc.GetHeight()));
  const double range = forward.distance;

  WindowProjection projection =
    CorridorProjection(start, forward.bearing, range, aspect, rc);

  const double map_scale = ForwardViewMapScale(rc, range, aspect);
  store.ScanVisibility(projection, 4, map_scale);
  const GeoBounds view_bounds =
    ForwardViewGeoBounds(start, forward.bearing, range, aspect);
  const Angle track = forward.bearing;

  for (const TopographyFile &file : store) {
    const std::lock_guard lock{file.mutex};

    if (!file.IsVisible(map_scale))
      continue;

    const Color color{file.GetColor()};
    const unsigned skip = file.GetSkipSteps(map_scale);

    for (const XShape &shape : file) {
      if (!view_bounds.Overlaps(shape.get_bounds()))
        continue;

      const auto type = shape.get_type();
      if (type == MS_SHAPE_NULL)
        continue;

      const auto lines = shape.GetLines();
      const ShapePoint *points = shape.GetPoints();

      if (type == MS_SHAPE_POINT) {
        unsigned offset = 0;
        for (const unsigned n_points : lines) {
          AppendSprites(sprites, start, track, range, aspect,
                        aircraft_alt, vertical_ref_alt,
                        map, file, points + offset, n_points, skip);
          offset += n_points;
        }
        continue;
      }

      if (type == MS_SHAPE_POLYGON) {
#ifdef ENABLE_OPENGL
        AppendPolygonFill(fill_vertices, start, track,
                          aircraft_alt, vertical_ref_alt,
                          map, color, file, shape, map_scale);
#endif
      }

      unsigned offset = 0;
      for (const unsigned n_points : lines) {
        AppendPolyline(line_vertices, start, track, range, aspect,
                       aircraft_alt, vertical_ref_alt,
                       map, file, points + offset, n_points, skip,
                       color);

        if (type == MS_SHAPE_POLYGON && n_points > 2) {
          const GeoPoint first = file.ToGeoPoint(points[offset]);
          const GeoPoint last =
            file.ToGeoPoint(points[offset + n_points - 1]);
          AppendDrapedSegment(line_vertices, start, track, range, aspect,
                              aircraft_alt, vertical_ref_alt,
                              map, color, last, first);
        }

        offset += n_points;
      }
    }
  }
}

} // namespace ForwardViewTopography
