// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ForwardView/ForwardViewAirspaceOverlay.hpp"

#ifdef ENABLE_OPENGL

#include "ForwardView/ForwardViewGeometry.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Geo/GeoVector.hpp"
#include "Look/AirspaceLook.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Topography/XShapePoint.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/opengl/Triangulate.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ForwardViewAirspace {

static constexpr double CORRIDOR_MARGIN = 500.;
static constexpr unsigned CIRCLE_SEGMENTS = 48;
static constexpr unsigned MAX_RING_POINTS = 256;
static constexpr unsigned MAX_FILL_VERTICES = 200000;
static constexpr unsigned MAX_OUTLINE_VERTICES = 300000;
static constexpr uint8_t FILL_ALPHA = 56;

static unsigned
MixColor(unsigned x, unsigned y, unsigned i) noexcept
{
  return (x * i + y * ((1 << 7) - i)) >> 7;
}

static Color
ApplyShading(Color color, int illum) noexcept
{
  if (illum == 0)
    return color;

  if (illum < 0) {
    const int x = std::min(96, -illum);
    return Color(MixColor(0, color.Red(), x),
                 MixColor(0, color.Green(), x),
                 MixColor(96, color.Blue(), x),
                 color.Alpha());
  }

  const int x = std::min(64, illum / 2);
  return Color(MixColor(255, color.Red(), x),
               MixColor(255, color.Green(), x),
               MixColor(16, color.Blue(), x),
               color.Alpha());
}

static int
FaceIllumination(double nx, double ny, double nz,
                 bool sun_active,
                 double sun_dir_x, double sun_dir_y, double sun_dir_z) noexcept
{
  if (!sun_active || !ForwardViewGeometry::SLOPE_SHADING_ENABLED)
    return 0;

  const double len = std::hypot(nx, std::hypot(ny, nz));
  if (len > 0.) {
    nx /= len;
    ny /= len;
    nz /= len;
  }

  const double dot = nx * sun_dir_x + ny * sun_dir_y + nz * sun_dir_z;
  const double lift = ForwardViewGeometry::SHADING_AMBIENT;
  const double shade = lift + (1. - lift) * std::clamp(dot, -1., 1.);
  return int(std::clamp((shade - 0.5) * ForwardViewGeometry::SHADING_CONTRAST,
                        -64., 64.));
}

static Color
ShadedFillColor(Color base, double nx, double ny, double nz,
                bool sun_active,
                double sun_dir_x, double sun_dir_y, double sun_dir_z) noexcept
{
  if (base.Alpha() == 0)
    return base;

  return ApplyShading(base, FaceIllumination(nx, ny, nz, sun_active,
                                             sun_dir_x, sun_dir_y, sun_dir_z));
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
RingCorridorVisible(const std::vector<double> &xs,
                    const std::vector<double> &ys,
                    double range, float aspect) noexcept
{
  if (xs.size() < 3 || xs.size() != ys.size())
    return false;

  double min_x = xs[0], max_x = xs[0];
  double min_y = ys[0], max_y = ys[0];
  for (std::size_t i = 1; i < xs.size(); ++i) {
    min_x = std::min(min_x, xs[i]);
    max_x = std::max(max_x, xs[i]);
    min_y = std::min(min_y, ys[i]);
    max_y = std::max(max_y, ys[i]);
  }

  const double far_half =
    ForwardViewGeometry::LateralHalfWidthAtDistance(range, aspect)
    + CORRIDOR_MARGIN;

  if (max_x < -CORRIDOR_MARGIN || min_x > range + CORRIDOR_MARGIN)
    return false;

  if (max_y < -far_half || max_y > far_half)
    return false;

  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (xs[i] >= -CORRIDOR_MARGIN && xs[i] <= range + CORRIDOR_MARGIN) {
      const double half_w =
        ForwardViewGeometry::LateralHalfWidthAtDistance(
          std::max(xs[i], 50.), aspect) + CORRIDOR_MARGIN;
      if (ys[i] >= -half_w && ys[i] <= half_w)
        return true;
    }
  }

  return false;
}

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
AppendLine(std::vector<ForwardViewTopography::Vertex> &vertices,
           double x1, double y1, double z1,
           double x2, double y2, double z2,
           Color color) noexcept
{
  AppendVertex(vertices, x1, y1, z1, color);
  AppendVertex(vertices, x2, y2, z2, color);
}

static void
AppendTriangle(std::vector<ForwardViewTopography::Vertex> &vertices,
               double x0, double y0, double z0,
               double x1, double y1, double z1,
               double x2, double y2, double z2,
               Color color) noexcept
{
  AppendVertex(vertices, x0, y0, z0, color);
  AppendVertex(vertices, x1, y1, z1, color);
  AppendVertex(vertices, x2, y2, z2, color);
}

static double
AltitudeZ(double msl, double ref_alt, double vertical_ref,
          double x, double y) noexcept
{
  return ForwardViewGeometry::RelativeTerrainZ(
    msl, ref_alt, vertical_ref, x, y, false);
}

static void
BuildCircleRing(const GeoPoint &center, double radius,
                const GeoPoint &origin, Angle track,
                std::vector<double> &xs, std::vector<double> &ys) noexcept
{
  xs.clear();
  ys.clear();
  xs.reserve(CIRCLE_SEGMENTS);
  ys.reserve(CIRCLE_SEGMENTS);

  for (unsigned i = 0; i < CIRCLE_SEGMENTS; ++i) {
    const Angle bearing = Angle::Degrees(360. * double(i) / double(CIRCLE_SEGMENTS));
    const GeoPoint p = GeoVector{radius, bearing}.EndPoint(center);
    double x = 0., y = 0.;
    if (!GeoToLocal(origin, track, p, x, y))
      continue;

    xs.push_back(x);
    ys.push_back(y);
  }
}

static void
BuildPolygonRing(const AbstractAirspace &as,
                 const GeoPoint &origin, Angle track,
                 std::vector<double> &xs, std::vector<double> &ys) noexcept
{
  xs.clear();
  ys.clear();

  const SearchPointVector &border = as.GetPoints();
  xs.reserve(border.size());
  ys.reserve(border.size());

  for (const SearchPoint &sp : border) {
    const GeoPoint &p = sp.GetLocation();
    double x = 0., y = 0.;
    if (!GeoToLocal(origin, track, p, x, y))
      continue;

    xs.push_back(x);
    ys.push_back(y);
  }

  if (xs.size() >= 2 &&
      std::hypot(xs.front() - xs.back(), ys.front() - ys.back()) < 1.) {
    xs.pop_back();
    ys.pop_back();
  }
}

static void
SubsampleRing(std::vector<double> &xs, std::vector<double> &ys) noexcept
{
  if (xs.size() <= MAX_RING_POINTS)
    return;

  std::vector<double> nx, ny;
  nx.reserve(MAX_RING_POINTS);
  ny.reserve(MAX_RING_POINTS);

  const std::size_t n = xs.size();
  for (unsigned i = 0; i < MAX_RING_POINTS; ++i) {
    const std::size_t idx = (i * n) / MAX_RING_POINTS;
    nx.push_back(xs[idx]);
    ny.push_back(ys[idx]);
  }

  xs.swap(nx);
  ys.swap(ny);
}

struct SlabPoint {
  double x, y, z_top, z_bottom;
};

static void
ExtrudeRing(std::vector<ForwardViewTopography::Vertex> &fill_vertices,
            std::vector<ForwardViewTopography::Vertex> &outline_vertices,
            const std::vector<double> &xs, const std::vector<double> &ys,
            double base_msl, double top_msl,
            double ref_alt, double vertical_ref,
            Color fill_color, Color outline_color,
            bool sun_active,
            double sun_dir_x, double sun_dir_y, double sun_dir_z) noexcept
{
  const unsigned n = unsigned(xs.size());
  if (n < 3)
    return;

  std::vector<SlabPoint> local;
  local.reserve(n);
  std::vector<ShapePoint> flat;
  flat.reserve(n);

  for (unsigned i = 0; i < n; ++i) {
    const double x = xs[i];
    const double y = ys[i];
    SlabPoint p{
      x, y,
      AltitudeZ(top_msl, ref_alt, vertical_ref, x, y),
      AltitudeZ(base_msl, ref_alt, vertical_ref, x, y),
    };
    local.push_back(p);
    flat.push_back(ShapePoint(float(x), float(y)));
  }

  if (fill_color.Alpha() > 0) {
    const Color top_color = ShadedFillColor(fill_color, 0., 0., 1.,
                                          sun_active, sun_dir_x, sun_dir_y,
                                          sun_dir_z);
    const Color bottom_color = ShadedFillColor(fill_color, 0., 0., -1.,
                                             sun_active, sun_dir_x, sun_dir_y,
                                             sun_dir_z);

    std::vector<GLushort> tri_indices((n - 2) * 3);
    const unsigned n_idx =
      PolygonToTriangles(flat.data(), n, tri_indices.data(), 1.f);

    auto append_cap = [&](unsigned i0, unsigned i1, unsigned i2,
                          bool top_cap) {
      const auto z_of = [top_cap](const SlabPoint &p) {
        return top_cap ? p.z_top : p.z_bottom;
      };
      const Color &cap_color = top_cap ? top_color : bottom_color;
      AppendTriangle(fill_vertices,
                     local[i0].x, local[i0].y, z_of(local[i0]),
                     local[i1].x, local[i1].y, z_of(local[i1]),
                     local[i2].x, local[i2].y, z_of(local[i2]),
                     cap_color);
    };

    if (n_idx >= 3) {
      for (unsigned t = 0; t + 2 < n_idx; t += 3) {
        const unsigned i0 = tri_indices[t];
        const unsigned i1 = tri_indices[t + 1];
        const unsigned i2 = tri_indices[t + 2];
        if (i0 >= n || i1 >= n || i2 >= n)
          continue;

        append_cap(i0, i1, i2, true);
        append_cap(i0, i2, i1, false);
      }
    } else {
      for (unsigned i = 1; i + 1 < n; ++i) {
        append_cap(0, i, i + 1, true);
        append_cap(0, i + 1, i, false);
      }
    }

    for (unsigned i = 0; i < n; ++i) {
      const unsigned j = (i + 1) % n;
      const double dx = local[j].x - local[i].x;
      const double dy = local[j].y - local[i].y;
      double nx = dy;
      double ny = -dx;
      const double nlen = std::hypot(nx, ny);
      if (nlen <= 0.)
        continue;

      nx /= nlen;
      ny /= nlen;

      const Color wall_color = ShadedFillColor(fill_color, nx, ny, 0.,
                                             sun_active, sun_dir_x, sun_dir_y,
                                             sun_dir_z);
      AppendTriangle(fill_vertices,
                     local[i].x, local[i].y, local[i].z_bottom,
                     local[j].x, local[j].y, local[j].z_bottom,
                     local[j].x, local[j].y, local[j].z_top,
                     wall_color);
      AppendTriangle(fill_vertices,
                     local[i].x, local[i].y, local[i].z_bottom,
                     local[j].x, local[j].y, local[j].z_top,
                     local[i].x, local[i].y, local[i].z_top,
                     wall_color);
    }
  }

  for (unsigned i = 0; i < n; ++i) {
    const unsigned j = (i + 1) % n;
    AppendLine(outline_vertices,
               local[i].x, local[i].y, local[i].z_top,
               local[j].x, local[j].y, local[j].z_top,
               outline_color);
    AppendLine(outline_vertices,
               local[i].x, local[i].y, local[i].z_bottom,
               local[j].x, local[j].y, local[j].z_bottom,
               outline_color);
    AppendLine(outline_vertices,
               local[i].x, local[i].y, local[i].z_bottom,
               local[i].x, local[i].y, local[i].z_top,
               outline_color);
  }
}

static bool
CacheMatches(const VolumeCache &cache, GeoPoint origin, Angle track,
             double range, float aspect, double ref_alt,
             Serial airspace_serial,
             bool sun_active,
             double sun_dir_x, double sun_dir_y, double sun_dir_z) noexcept
{
  if (!cache.valid)
    return false;

  if (!track.CompareRoughly(cache.track) ||
      cache.range != range || cache.aspect != aspect ||
      cache.ref_alt != ref_alt ||
      cache.airspace_serial != airspace_serial ||
      cache.sun_active != sun_active)
    return false;

  if (sun_active &&
      (std::abs(cache.sun_dir_x - float(sun_dir_x)) > 0.05f ||
       std::abs(cache.sun_dir_y - float(sun_dir_y)) > 0.05f ||
       std::abs(cache.sun_dir_z - float(sun_dir_z)) > 0.05f))
    return false;

  const GeoVector rel(cache.anchor, origin);
  return rel.distance <= ForwardViewGeometry::SMOOTH_MOTION_REPAINT_DIST;
}

void
BuildVolumes(std::vector<ForwardViewTopography::Vertex> &fill_vertices,
             std::vector<ForwardViewTopography::Vertex> &outline_vertices,
             VolumeCache &cache,
             const Airspaces &database,
             const AirspaceLook &look,
             const AirspaceRendererSettings &renderer_settings,
             const AirspaceComputerSettings &computer_settings,
             const AircraftState &aircraft,
             GeoPoint origin, Angle track, double range, float aspect,
             double ref_alt, double vertical_ref_alt,
             double sun_dir_x, double sun_dir_y, double sun_dir_z,
             bool sun_active,
             const PixelRect &rc) noexcept
{
  (void)rc;

  if (database.IsEmpty()) {
    cache.valid = false;
    fill_vertices.clear();
    outline_vertices.clear();
    return;
  }

  const Serial airspace_serial = database.GetSerial();
  if (CacheMatches(cache, origin, track, range, aspect, ref_alt,
                   airspace_serial, sun_active,
                   sun_dir_x, sun_dir_y, sun_dir_z)) {
    fill_vertices = cache.fill_vertices;
    outline_vertices = cache.outline_vertices;
    return;
  }

  fill_vertices.clear();
  outline_vertices.clear();
  fill_vertices.reserve(8192);
  outline_vertices.reserve(16384);

  const GeoPoint mid = GeoVector{range * 0.5, track}.EndPoint(origin);
  const double query_radius =
    std::hypot(range,
               ForwardViewGeometry::LateralHalfWidthAtDistance(range, aspect))
    * 1.2;

  const AirspaceVisibility visible(computer_settings, renderer_settings,
                                   aircraft);

  std::vector<double> xs, ys;

  for (const auto &item : database.QueryWithinRange(mid, query_radius)) {
    const ConstAirspacePtr ptr = item.GetAirspacePtr();
    if (ptr == nullptr)
      continue;

    const AbstractAirspace &as = *ptr;
    if (!visible(as))
      continue;

    const AirspaceClass type = as.GetTypeOrClass();
    const auto &class_settings = renderer_settings.classes[type];

    const double base_msl = as.GetBaseAltitude(aircraft);
    const double top_msl = as.GetTopAltitude(aircraft);
    if (top_msl <= base_msl + 1.)
      continue;

    if (as.GetShape() == AbstractAirspace::Shape::CIRCLE) {
      const auto &circle = static_cast<const AirspaceCircle &>(as);
      BuildCircleRing(circle.GetCenter(), circle.GetRadius(),
                      origin, track, xs, ys);
    } else
      BuildPolygonRing(as, origin, track, xs, ys);

    if (xs.size() < 3)
      continue;

    SubsampleRing(xs, ys);
    if (!RingCorridorVisible(xs, ys, range, aspect))
      continue;

    const AirspaceClassLook &class_look = look.classes[type];
    const bool draw_fill =
      class_settings.fill_mode !=
      AirspaceClassRendererSettings::FillMode::NONE;
    const Color fill_color = draw_fill
      ? class_look.fill_color.WithAlpha(FILL_ALPHA)
      : Color(0, 0, 0, 0);
    const Color outline_color = class_look.border_pen.GetColor().WithAlpha(255);

    ExtrudeRing(fill_vertices, outline_vertices, xs, ys,
                base_msl, top_msl, ref_alt, vertical_ref_alt,
                fill_color, outline_color,
                sun_active, sun_dir_x, sun_dir_y, sun_dir_z);

    if (fill_vertices.size() >= MAX_FILL_VERTICES ||
        outline_vertices.size() >= MAX_OUTLINE_VERTICES)
      break;
  }

  cache.valid = true;
  cache.anchor = origin;
  cache.track = track;
  cache.range = range;
  cache.aspect = aspect;
  cache.ref_alt = ref_alt;
  cache.airspace_serial = airspace_serial;
  cache.sun_active = sun_active;
  cache.sun_dir_x = float(sun_dir_x);
  cache.sun_dir_y = float(sun_dir_y);
  cache.sun_dir_z = float(sun_dir_z);
  cache.fill_vertices = fill_vertices;
  cache.outline_vertices = outline_vertices;
}

} // namespace ForwardViewAirspace

#endif
