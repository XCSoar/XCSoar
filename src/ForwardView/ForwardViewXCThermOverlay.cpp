// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ForwardView/ForwardViewXCThermOverlay.hpp"

#if defined(ENABLE_OPENGL) && defined(HAVE_HTTP)

#include "ForwardView/ForwardViewGeometry.hpp"
#include "ForwardView/ForwardViewTerrain.hpp"
#include "Weather/xctherm/XCThermGeoJSONOverlay.hpp"
#include "Weather/xctherm/XCThermMapOverlay.hpp"
#include "Terrain/Height.hpp"
#include "Geo/GeoVector.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/opengl/Triangulate.hpp"
#include "Topography/XShapePoint.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ForwardViewXCTherm {

static constexpr double CORRIDOR_MARGIN = 500.;
static constexpr double SLAB_HALF_THICKNESS_AMSL = 100.;
static constexpr double SLAB_HALF_THICKNESS_AGL = 50.;
static constexpr unsigned MAX_VOLUME_VERTICES = 300000;

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
  if (x < -CORRIDOR_MARGIN || x > range + CORRIDOR_MARGIN)
    return false;

  const double half_w =
    ForwardViewGeometry::LateralHalfWidthAtDistance(std::max(x, 50.), aspect)
    + CORRIDOR_MARGIN;
  return y >= -half_w && y <= half_w;
}

static unsigned
EffectiveRingSize(const XCThermGeoJSON::Ring &ring) noexcept
{
  unsigned n = unsigned(ring.size());
  if (n >= 2) {
    const GeoVector d(ring.front(), ring.back());
    if (d.distance < 1.)
      --n;
  }
  return n;
}

static bool
RingCorridorVisible(const XCThermGeoJSON::Ring &ring,
                    const GeoPoint &origin, Angle track,
                    double range, float aspect) noexcept
{
  const unsigned n = EffectiveRingSize(ring);
  if (n < 3)
    return false;

  double min_x = 0., max_x = 0., min_y = 0., max_y = 0.;
  bool any = false;

  for (unsigned i = 0; i < n; ++i) {
    double x = 0., y = 0.;
    if (!GeoToLocal(origin, track, ring[i], x, y))
      continue;

    if (!any) {
      min_x = max_x = x;
      min_y = max_y = y;
      any = true;
    } else {
      min_x = std::min(min_x, x);
      max_x = std::max(max_x, x);
      min_y = std::min(min_y, y);
      max_y = std::max(max_y, y);
    }
  }

  if (!any)
    return false;

  const double far_half =
    ForwardViewGeometry::LateralHalfWidthAtDistance(range, aspect)
    + CORRIDOR_MARGIN;

  if (max_x < -CORRIDOR_MARGIN || min_x > range + CORRIDOR_MARGIN)
    return false;

  if (max_y < -far_half || min_y > far_half)
    return false;

  for (unsigned i = 0; i < n; ++i) {
    double x = 0., y = 0.;
    if (GeoToLocal(origin, track, ring[i], x, y) &&
        InViewCorridor(x, y, range, aspect))
      return true;
  }

  return false;
}

static double
SlabMslAt(const GeoPoint &geo, const RasterMap *terrain_map,
          unsigned altitude_m, bool is_agl) noexcept
{
  if (!is_agl)
    return double(altitude_m);

  if (terrain_map == nullptr)
    return double(altitude_m);

  const TerrainHeight h = ForwardViewTerrain::HeightAt(*terrain_map, geo);
  if (h.IsInvalid())
    return double(altitude_m);

  return h.GetValueOr0() + double(altitude_m);
}

static double
SlabPlaneZ(double msl_amsl, double ref_alt, double vertical_ref,
           double x, double y, double offset_m) noexcept
{
  return ForwardViewGeometry::RelativeTerrainZ(
    msl_amsl + offset_m, ref_alt, vertical_ref, x, y, false);
}

struct LocalSlabPoint {
  double x, y;
  double z_top, z_bottom;
};

static void
AppendVertex(std::vector<ForwardViewTopography::Vertex> &vertices,
             double x, double geo_y, double z, Color color) noexcept
{
  vertices.push_back({
    float(x),
    float(geo_y),
    float(z),
    float(color.Red()) / 255.f,
    float(color.Green()) / 255.f,
    float(color.Blue()) / 255.f,
    float(color.Alpha()) / 255.f,
  });
}

static void
AppendTriangle(std::vector<ForwardViewTopography::Vertex> &vertices,
               const LocalSlabPoint &a, const LocalSlabPoint &b,
               const LocalSlabPoint &c, Color color,
               bool top_cap) noexcept
{
  const auto z_of = [top_cap](const LocalSlabPoint &p) {
    return top_cap ? p.z_top : p.z_bottom;
  };

  AppendVertex(vertices, a.x, a.y, z_of(a), color);
  AppendVertex(vertices, b.x, b.y, z_of(b), color);
  AppendVertex(vertices, c.x, c.y, z_of(c), color);
}

static void
AppendWallQuad(std::vector<ForwardViewTopography::Vertex> &vertices,
               const LocalSlabPoint &a, const LocalSlabPoint &b,
               Color color) noexcept
{
  AppendVertex(vertices, a.x, a.y, a.z_bottom, color);
  AppendVertex(vertices, b.x, b.y, b.z_bottom, color);
  AppendVertex(vertices, b.x, b.y, b.z_top, color);

  AppendVertex(vertices, a.x, a.y, a.z_bottom, color);
  AppendVertex(vertices, b.x, b.y, b.z_top, color);
  AppendVertex(vertices, a.x, a.y, a.z_top, color);
}

static bool
ExtrudeRing(std::vector<ForwardViewTopography::Vertex> &vertices,
            const XCThermGeoJSON::Ring &ring,
            const GeoPoint &origin, Angle track,
            double ref_alt, double vertical_ref,
            const RasterMap *terrain_map,
            unsigned altitude_m, bool is_agl,
            double half_thickness, Color color,
            double range, float aspect) noexcept
{
  const unsigned n = EffectiveRingSize(ring);
  if (n < 3)
    return false;

  if (!RingCorridorVisible(ring, origin, track, range, aspect))
    return false;

  std::vector<LocalSlabPoint> local;
  local.reserve(n);
  std::vector<ShapePoint> flat;
  flat.reserve(n);

  for (unsigned i = 0; i < n; ++i) {
    double x = 0., y = 0.;
    if (!GeoToLocal(origin, track, ring[i], x, y))
      return false;

    const double msl =
      SlabMslAt(ring[i], terrain_map, altitude_m, is_agl);
    LocalSlabPoint p{
      x, y,
      SlabPlaneZ(msl, ref_alt, vertical_ref, x, y, half_thickness),
      SlabPlaneZ(msl, ref_alt, vertical_ref, x, y, -half_thickness),
    };
    local.push_back(p);
    flat.push_back(ShapePoint(float(x), float(y)));
  }

  std::vector<GLushort> tri_indices((n - 2) * 3);
  const unsigned n_idx =
    PolygonToTriangles(flat.data(), n, tri_indices.data(), 1.f);

  unsigned n_triangles = 0;
  if (n_idx >= 3) {
    for (unsigned t = 0; t + 2 < n_idx; t += 3) {
      const unsigned i0 = tri_indices[t];
      const unsigned i1 = tri_indices[t + 1];
      const unsigned i2 = tri_indices[t + 2];
      if (i0 >= n || i1 >= n || i2 >= n)
        continue;

      AppendTriangle(vertices, local[i0], local[i1], local[i2], color, true);
      AppendTriangle(vertices, local[i0], local[i2], local[i1], color, false);
      ++n_triangles;
    }
  }

  if (n_triangles == 0) {
    for (unsigned i = 1; i + 1 < n; ++i) {
      AppendTriangle(vertices, local[0], local[i], local[i + 1], color, true);
      AppendTriangle(vertices, local[0], local[i + 1], local[i], color, false);
      ++n_triangles;
    }
  }

  if (n_triangles == 0)
    return false;

  for (unsigned i = 0; i < n; ++i) {
    const unsigned j = (i + 1) % n;
    AppendWallQuad(vertices, local[i], local[j], color);
  }

  return true;
}

static bool
CacheMatches(const VolumeCache &cache, GeoPoint origin, Angle track,
             double range, float aspect, double ref_alt,
             const XCThermGeoJSONOverlay &overlay) noexcept
{
  if (!cache.valid)
    return false;

  if (!track.CompareRoughly(cache.track) ||
      cache.range != range || cache.aspect != aspect ||
      cache.ref_alt != ref_alt ||
      overlay.GetParameter() != cache.parameter ||
      overlay.GetForecastUtc() != cache.forecast_utc)
    return false;

  const GeoVector rel(cache.anchor, origin);
  if (rel.distance > ForwardViewGeometry::SMOOTH_MOTION_REPAINT_DIST)
    return false;

  return true;
}

void
BuildVolumes(std::vector<ForwardViewTopography::Vertex> &vertices,
             VolumeCache &cache,
             GeoPoint origin, Angle track, double range, float aspect,
             double ref_alt, double vertical_ref_alt,
             const RasterMap *terrain_map,
             const PixelRect &rc) noexcept
{
  (void)rc;

  const auto *overlay = XCTherm::GetMapOverlay();
  if (overlay == nullptr || !overlay->HasData()) {
    cache.valid = false;
    vertices.clear();
    return;
  }

  if (CacheMatches(cache, origin, track, range, aspect, ref_alt, *overlay)) {
    vertices = cache.vertices;
    return;
  }

  XCThermGeoJSONOverlay::ForecastSnapshot snapshot;
  if (!overlay->TryCopyForecast(snapshot))
    return;

  unsigned altitude_m = 0;
  bool is_agl = false;
  if (!XCTherm::FindLayerByApiParameter(snapshot.parameter,
                                        altitude_m, is_agl))
    return;

  const double half_thickness =
    is_agl ? SLAB_HALF_THICKNESS_AGL : SLAB_HALF_THICKNESS_AMSL;

  vertices.clear();
  vertices.reserve(65536);

  for (const auto &band : snapshot.forecast.bands) {
    const Color fill_color =
      XCThermGeoJSONOverlay::BandFillColor(band.min_ms, band.max_ms);

    for (const auto &polygon : band.polygons) {
      if (polygon.empty())
        continue;

      ExtrudeRing(vertices, polygon[0], origin, track,
                  ref_alt, vertical_ref_alt, terrain_map,
                  altitude_m, is_agl, half_thickness, fill_color,
                  range, aspect);

      if (vertices.size() >= MAX_VOLUME_VERTICES)
        break;
    }

    if (vertices.size() >= MAX_VOLUME_VERTICES)
      break;
  }

  cache.valid = true;
  cache.anchor = origin;
  cache.track = track;
  cache.range = range;
  cache.aspect = aspect;
  cache.ref_alt = ref_alt;
  cache.parameter = snapshot.parameter;
  cache.forecast_utc = snapshot.forecast_utc;
  cache.forecast_coords = snapshot.forecast.TotalCoords();
  cache.vertices = vertices;
}

} // namespace ForwardViewXCTherm

#endif
