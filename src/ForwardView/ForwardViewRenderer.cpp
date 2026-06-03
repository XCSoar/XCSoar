// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ForwardViewRenderer.hpp"
#include "ForwardView/ForwardViewGeometry.hpp"
#include "ForwardView/ForwardViewTerrain.hpp"
#include "ForwardView/ForwardViewTopographyTexture.hpp"
#include "Renderer/GradientRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/CrossSectionLook.hpp"
#include "Look/TopographyLook.hpp"
#include "Look/Colors.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Terrain/RasterRenderer.hpp"
#include "Terrain/Height.hpp"
#include "ui/canvas/Ramp.hpp"
#include "MapSettings.hpp"
#include "Language/Language.hpp"
#include "Topography/ForwardViewTopographyOverlay.hpp"
#include "Topography/TopographyStore.hpp"
#include "ForwardView/ForwardViewAirspaceOverlay.hpp"
#ifdef HAVE_HTTP
#include "ForwardView/ForwardViewXCThermOverlay.hpp"
#endif
#include "Screen/Layout.hpp"
#include "Interface.hpp"
#include "NMEA/Aircraft.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/Task/Stats/ElementStat.hpp"
#include "Engine/Task/Stats/TaskStats.hpp"
#include "Math/SunEphemeris.hpp"
#include "Math/Angle.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#include "ui/canvas/opengl/Shaders.hpp"
#include "ui/canvas/opengl/Program.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/opengl/System.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

ForwardViewRenderer::ForwardViewRenderer(const CrossSectionLook &_look,
                                         [[maybe_unused]] const AirspaceLook &_airspace_look,
                                         const TopographyLook &_topography_look,
                                         const bool _inverse) noexcept
  :look(_look), airspace_look(_airspace_look),
   topography_look(_topography_look), inverse(_inverse),
   topo_texture(_topography_look) {}

void
ForwardViewRenderer::ReadBlackboard(const MoreData &basic,
                                      const DerivedInfo &calculated,
                                      const GlideSettings &_glide_settings,
                                      const GlidePolar &_glide_polar,
                                      const MapSettings &map_settings,
                                      RoughTimeDelta _utc_offset) noexcept
{
  gps_info = basic;
  calculated_info = calculated;
  glide_settings = _glide_settings;
  glide_polar = _glide_polar;
  terrain_settings = map_settings.terrain;
  airspace_settings = map_settings.airspace;
  airspace_enabled = map_settings.airspace.enable;
  topography_enabled = map_settings.topography_enabled;
  utc_offset = _utc_offset;
}

void
ForwardViewRenderer::SetView(GeoPoint _start, Angle track,
                             double range) noexcept
{
  start = _start;

  if (!smoothed_track_valid) {
    smoothed_track = track;
    smoothed_track_valid = true;
  } else
    smoothed_track = smoothed_track.Fraction(track, 0.25);

  forward = GeoVector{range, smoothed_track};
}

void
ForwardViewRenderer::SetMotionTarget(GeoPoint fix, Angle track,
                                     double ground_speed,
                                     double range) noexcept
{
  motion_fix = fix;
  motion_track = track;
  motion_speed = std::max(ground_speed, 0.);
  view_range = range;
  motion_valid = fix.IsValid();
  motion_fix_clock.Update();
  last_repaint_start.SetInvalid();
  (void)TickMotionForRepaint();
}

void
ForwardViewRenderer::TickMotion() noexcept
{
  if (!motion_valid || !motion_fix.IsValid())
    return;

  const auto elapsed = motion_fix_clock.Elapsed();
  if (elapsed.count() < 0)
    return;

  double dt = std::chrono::duration<double>(elapsed).count();
  dt = std::min(dt, ForwardViewGeometry::SMOOTH_MOTION_MAX_EXTRAPOLATION);

  GeoPoint display = motion_fix;
  if (motion_speed > 1. && dt > 0.)
    display = GeoVector{motion_speed * dt, motion_track}.EndPoint(motion_fix);

  SetView(display, motion_track, view_range);
}

bool
ForwardViewRenderer::TickMotionForRepaint() noexcept
{
  TickMotion();

  if (!start.IsValid())
    return false;

  if (!last_repaint_start.IsValid()) {
    last_repaint_start = start;
    return true;
  }

  const double moved = last_repaint_start.DistanceS(start);
  if (moved < ForwardViewGeometry::SMOOTH_MOTION_REPAINT_DIST)
    return false;

  last_repaint_start = start;
  return true;
}

#ifdef ENABLE_OPENGL

static constexpr double SUN_DISTANCE = 90000.;
static constexpr double SUN_RADIUS = 350.;
static constexpr double SKY_RADIUS =
  double(ForwardViewGeometry::CAMERA_FAR_CLIP);
static constexpr Angle DEFAULT_SUN_SHADING_AZIMUTH = Angle::Degrees(-45);

struct SunLight {
  double dir_x = 0.;
  double dir_y = 0.;
  double dir_z = 1.;
  bool visible = false;
  bool active = false;
};

static constexpr unsigned
MixColor(unsigned x, unsigned y, unsigned i) noexcept
{
  return (x * i + y * ((1 << 7) - i)) >> 7;
}

static Color
ApplyTerrainShading(Color color, int illum) noexcept
{
  if (illum == 0)
    return color;

  if (illum < 0) {
    const int x = std::min(96, -illum);
    return Color(MixColor(0, color.Red(), x),
                 MixColor(0, color.Green(), x),
                 MixColor(96, color.Blue(), x));
  }

  const int x = std::min(64, illum / 2);
  return Color(MixColor(255, color.Red(), x),
               MixColor(255, color.Green(), x),
               MixColor(16, color.Blue(), x));
}

static SunLight
GetForwardViewSunLight(const DerivedInfo &calculated,
                       const MoreData &basic,
                       RoughTimeDelta utc,
                       Angle track_bearing) noexcept
{
  SunLight sun;

  if (!ForwardViewGeometry::SLOPE_SHADING_ENABLED)
    return sun;

  sun.active = true;

  Angle azimuth = DEFAULT_SUN_SHADING_AZIMUTH + track_bearing;
  Angle elevation = Angle::Degrees(45.);

  if (calculated.sun_data_available &&
      basic.location_available &&
      basic.date_time_utc.IsPlausible()) {
    azimuth = calculated.sun_azimuth;
    elevation = SunEphemeris::CalcElevation(basic.location,
                                            basic.date_time_utc,
                                            utc);
    sun.visible = elevation > Angle::Degrees(-0.5);
    if (elevation < Angle::Degrees(ForwardViewGeometry::MIN_SUN_ELEVATION_DEG))
      elevation = Angle::Degrees(ForwardViewGeometry::MIN_SUN_ELEVATION_DEG);
  } else
    sun.visible = true;

  const Angle relative = azimuth - track_bearing;
  sun.dir_x = elevation.cos() * relative.cos();
  sun.dir_y = elevation.cos() * relative.sin();
  sun.dir_z = elevation.sin();

  const double len = std::hypot(sun.dir_x, std::hypot(sun.dir_y, sun.dir_z));
  if (len > 0.) {
    sun.dir_x /= len;
    sun.dir_y /= len;
    sun.dir_z /= len;
  }

  return sun;
}

static int
VertexIllumination(double nx, double ny, double nz,
                   const SunLight &sun) noexcept
{
  if (!sun.active)
    return 0;

  const double dot = nx * sun.dir_x + ny * sun.dir_y + nz * sun.dir_z;
  const double lift = ForwardViewGeometry::SHADING_AMBIENT;
  const double shade = lift + (1. - lift) * std::clamp(dot, -1., 1.);
  return int(std::clamp((shade - 0.5) * ForwardViewGeometry::SHADING_CONTRAST,
                        -64., 64.));
}

static int
TerrainIllumination(double nx, double ny, double nz,
                    double display_x, double display_y, double mesh_z,
                    double scroll_dx, double scroll_dy,
                    const SunLight &sun,
                    const std::vector<double> &xs,
                    const std::vector<std::vector<double>> &ys,
                    const std::vector<std::vector<double>> &heights,
                    unsigned dist_count, unsigned lateral_count,
                    double sample_step) noexcept
{
  int illum = VertexIllumination(nx, ny, nz, sun);

  if (!ForwardViewGeometry::CAST_SHADOWS_ENABLED || !sun.active)
    return illum;

  if (ForwardViewTerrain::PointCastShadow(display_x + scroll_dx,
                                          display_y + scroll_dy, mesh_z,
                                          sun.dir_x, sun.dir_y, sun.dir_z,
                                          xs, ys, heights,
                                          dist_count, lateral_count,
                                          sample_step))
    illum = std::min(illum, ForwardViewGeometry::CAST_SHADOW_ILLUM);

  return illum;
}

static void
ComputeVertexNormal(unsigned i, unsigned j,
                    unsigned dist_count, unsigned lateral_count,
                    const std::vector<double> &xs,
                    const std::vector<std::vector<double>> &ys,
                    const std::vector<std::vector<double>> &heights,
                    double &nx, double &ny, double &nz) noexcept
{
  double dzdx = 0., dzdy = 0.;

  if (i > 0 && i + 1 < dist_count) {
    const double dx = xs[i + 1] - xs[i - 1];
    if (std::abs(dx) > 0.01)
      dzdx = (heights[i + 1][j] - heights[i - 1][j]) / dx;
  } else if (i + 1 < dist_count) {
    const double dx = xs[i + 1] - xs[i];
    if (std::abs(dx) > 0.01)
      dzdx = (heights[i + 1][j] - heights[i][j]) / dx;
  } else if (i > 0) {
    const double dx = xs[i] - xs[i - 1];
    if (std::abs(dx) > 0.01)
      dzdx = (heights[i][j] - heights[i - 1][j]) / dx;
  }

  if (j > 0 && j + 1 < lateral_count) {
    const double dy = ys[i][j + 1] - ys[i][j - 1];
    if (std::abs(dy) > 0.01)
      dzdy = (heights[i][j + 1] - heights[i][j - 1]) / dy;
  } else if (j + 1 < lateral_count) {
    const double dy = ys[i][j + 1] - ys[i][j];
    if (std::abs(dy) > 0.01)
      dzdy = (heights[i][j + 1] - heights[i][j]) / dy;
  } else if (j > 0) {
    const double dy = ys[i][j] - ys[i][j - 1];
    if (std::abs(dy) > 0.01)
      dzdy = (heights[i][j] - heights[i][j - 1]) / dy;
  }

  nx = -dzdx;
  ny = -dzdy;
  nz = 1.;

  const double len = std::hypot(nx, std::hypot(ny, nz));
  if (len > 0.) {
    nx /= len;
    ny /= len;
    nz /= len;
  }
}

static Color
LerpColor(Color a, Color b, double t) noexcept
{
  t = std::clamp(t, 0., 1.);
  return Color(unsigned(a.Red() + t * (int(b.Red()) - int(a.Red()))),
               unsigned(a.Green() + t * (int(b.Green()) - int(a.Green()))),
               unsigned(a.Blue() + t * (int(b.Blue()) - int(a.Blue()))),
               unsigned(a.Alpha() + t * (int(b.Alpha()) - int(a.Alpha()))));
}

static constexpr unsigned TERRAIN_HEIGHT_SCALE = 4;

static Color
LookupMapTerrainColor(const TerrainHeight &h, double msl,
                      unsigned ramp_index) noexcept
{
  if (h.IsWater())
    return Color(85, 160, 255);

  if (h.IsSpecial())
    return COLOR_WHITE;

  const unsigned scaled =
    std::min(254u, unsigned(std::max(0., msl)) >> TERRAIN_HEIGHT_SCALE);
  const RGB8Color rgb = ColorRampLookup(int(scaled << TERRAIN_HEIGHT_SCALE),
                                      GetTerrainColorRamp(ramp_index),
                                      NUM_COLOR_RAMP_LEVELS, 2);
  return Color(rgb.Red(), rgb.Green(), rgb.Blue());
}

static Color
TerrainColor(const TerrainHeight &h, double msl, unsigned ramp_index,
             float ramp_weight = 1.f) noexcept
{
  const Color ramp = LookupMapTerrainColor(h, msl, ramp_index);
  if (ramp_weight >= 1.f || h.IsWater() || h.IsSpecial())
    return ramp;

  return LerpColor(COLOR_FORWARD_VIEW_LAND, ramp, double(ramp_weight));
}

static Color
SkyHorizonColor(Color sky) noexcept
{
  return Color(std::min(255u, unsigned(sky.Red()) + 40u),
               std::min(255u, unsigned(sky.Green()) + 50u),
               std::min(255u, unsigned(sky.Blue()) + 30u));
}

static Color
SkyZenithColor(Color sky) noexcept
{
  return Color(std::min(255u, unsigned(sky.Red()) + 15u),
               std::min(255u, unsigned(sky.Green()) + 25u),
               std::min(255u, unsigned(sky.Blue()) + 10u));
}

static Color
ApplyHorizonComposite(Color color,
                      ForwardViewGeometry::HorizonComposite comp) noexcept
{
  if (comp.alpha < 1.) {
    const unsigned alpha = unsigned(std::clamp(
      comp.alpha * double(color.Alpha()), 0., 255.));
    color = color.WithAlpha(alpha);
  }

  return color;
}

static Color
ShadedTerrainColor(const TerrainHeight &h, double msl,
                   int illum, double x, double y, double z,
                   double eye_z, double ref_alt,
                   unsigned ramp_index,
                   float ramp_weight = 1.f) noexcept
{
  Color color = ApplyTerrainShading(
    TerrainColor(h, msl, ramp_index, ramp_weight), illum);

  const ForwardViewGeometry::HorizonComposite comp =
    ForwardViewGeometry::HorizonCompositeAt(z, x, y, eye_z, ref_alt);
  return ApplyHorizonComposite(color, comp);
}

static Color
GridLineColor(double x, double y, double z,
              double eye_z, double ref_alt) noexcept
{
  const double slant = ForwardViewGeometry::SlantRange(x, y, z, 0.);
  const double strength = ForwardViewGeometry::GridLineStrength(slant);
  if (strength <= 0.01)
    return COLOR_FORWARD_VIEW_GRID.WithAlpha(0);

  const ForwardViewGeometry::HorizonComposite comp =
    ForwardViewGeometry::HorizonCompositeAt(z, x, y, eye_z, ref_alt);
  const unsigned alpha = unsigned(std::clamp(
    72. * strength * comp.alpha,
    0., 72.));
  return COLOR_FORWARD_VIEW_GRID.WithAlpha(alpha);
}

static bool
UsesHorizonFade(Color color) noexcept
{
  return color.Alpha() < 250;
}

static bool
UsesHorizonFadeVertex(const ForwardViewRenderer::Vertex3D &v) noexcept
{
  return v.a < (250.f / 255.f);
}

static ForwardViewRenderer::Vertex3D
MakeVertex(double x, double y, double z, Color color) noexcept
{
  return {
    GLfloat(x), GLfloat(y), GLfloat(z),
    GLfloat(color.Red()) / 255.f,
    GLfloat(color.Green()) / 255.f,
    GLfloat(color.Blue()) / 255.f,
    GLfloat(color.Alpha()) / 255.f,
  };
}

static ForwardViewRenderer::Vertex3D
MakeDisplayVertex(double x, double geo_lateral, double z,
                  Color color) noexcept
{
  return MakeVertex(x, ForwardViewGeometry::DisplayLateral(geo_lateral),
                    z, color);
}

static void
AppendTriangle(std::vector<ForwardViewRenderer::Vertex3D> &vertices,
               ForwardViewRenderer::Vertex3D a,
               ForwardViewRenderer::Vertex3D b,
               ForwardViewRenderer::Vertex3D c) noexcept
{
  vertices.push_back(a);
  vertices.push_back(b);
  vertices.push_back(c);
}

static void
AppendQuad(std::vector<ForwardViewRenderer::Vertex3D> &vertices,
           ForwardViewRenderer::Vertex3D a,
           ForwardViewRenderer::Vertex3D b,
           ForwardViewRenderer::Vertex3D c,
           ForwardViewRenderer::Vertex3D d) noexcept
{
  AppendTriangle(vertices, a, b, c);
  AppendTriangle(vertices, a, c, d);
}

static void
AppendQuadSplit(std::vector<ForwardViewRenderer::Vertex3D> &opaque,
                std::vector<ForwardViewRenderer::Vertex3D> &fade,
                ForwardViewRenderer::Vertex3D a,
                ForwardViewRenderer::Vertex3D b,
                ForwardViewRenderer::Vertex3D c,
                ForwardViewRenderer::Vertex3D d) noexcept
{
  if (UsesHorizonFadeVertex(a) || UsesHorizonFadeVertex(b) ||
      UsesHorizonFadeVertex(c) || UsesHorizonFadeVertex(d))
    AppendQuad(fade, a, b, c, d);
  else
    AppendQuad(opaque, a, b, c, d);
}

static void
AppendLine(std::vector<ForwardViewRenderer::Vertex3D> &vertices,
           double x1, double geo_y1, double z1,
           double x2, double geo_y2, double z2,
           Color color) noexcept
{
  vertices.push_back(MakeDisplayVertex(x1, geo_y1, z1, color));
  vertices.push_back(MakeDisplayVertex(x2, geo_y2, z2, color));
}

static void
AppendLineSplit(std::vector<ForwardViewRenderer::Vertex3D> &opaque,
                std::vector<ForwardViewRenderer::Vertex3D> &fade,
                double x1, double geo_y1, double z1,
                double x2, double geo_y2, double z2,
                Color c1, Color c2) noexcept
{
  const Color ca = c1.Alpha() <= c2.Alpha() ? c1 : c2;
  auto &out = UsesHorizonFade(ca) ? fade : opaque;
  AppendLine(out, x1, geo_y1, z1, x2, geo_y2, z2, c1);
}

void
ForwardViewRenderer::EnsureTerrainMeshCache(double ref_alt,
                                             float aspect,
                                             bool *rebuilt) const noexcept
{
  if (rebuilt != nullptr)
    *rebuilt = false;

  if (terrain == nullptr || !start.IsValid() || !forward.IsValid())
    return;

  const Angle track = forward.bearing;
  const double range = forward.distance;

  Serial serial;
  {
    RasterTerrain::Lease map(*terrain);
    serial = map->GetSerial();
  }

  if (terrain_mesh.valid &&
      serial == terrain_mesh.terrain_serial &&
      terrain_mesh.ramp == terrain_settings.ramp &&
      track.CompareRoughly(terrain_mesh.track) &&
      range == terrain_mesh.range &&
      aspect == terrain_mesh.aspect) {
    const GeoVector rel(terrain_mesh.anchor, start);
    const Angle rel_bearing = rel.bearing - terrain_mesh.track;
    const double along = rel.distance * rel_bearing.cos();
    if (along >= 0. && along < terrain_mesh.range * 0.7)
      return;
  }

  if (rebuilt != nullptr)
    *rebuilt = true;

  terrain_mesh.valid = false;
  terrain_mesh.anchor = start;
  terrain_mesh.track = track;
  terrain_mesh.range = range;
  terrain_mesh.aspect = aspect;
  terrain_mesh.terrain_serial = serial;
  terrain_mesh.vertical_ref_alt = ref_alt;
  terrain_mesh.ramp = terrain_settings.ramp;

  const GeoPoint origin = terrain_mesh.anchor;
  const Angle lateral_bearing = track + Angle::Degrees(90.);

  RasterTerrain::Lease map(*terrain);
  const double dem_cell = ForwardViewGeometry::ClampDemCellSpacing(
    map->PixelDistance(origin, 1));
  terrain_mesh.dem_cell_spacing = dem_cell;

  const double max_half_width =
    ForwardViewGeometry::LateralHalfWidthAtDistance(
      std::max(range, ForwardViewGeometry::MIN_TERRAIN_DIST), aspect);
  terrain_mesh.dist_count =
    ForwardViewGeometry::DistSampleCount(range, dem_cell);
  terrain_mesh.lateral_count =
    ForwardViewGeometry::LateralSampleCount(max_half_width, dem_cell);

  const double dist_span =
    std::max(range - ForwardViewGeometry::MIN_TERRAIN_DIST, dem_cell);
  const double dist_step =
    dist_span / double(terrain_mesh.dist_count - 1);

  terrain_mesh.xs.resize(terrain_mesh.dist_count);
  terrain_mesh.ys.resize(terrain_mesh.dist_count);
  terrain_mesh.raw_heights.resize(terrain_mesh.dist_count);
  terrain_mesh.terrain_heights.resize(terrain_mesh.dist_count);

  for (unsigned i = 0; i < terrain_mesh.dist_count; ++i) {
    const double ahead = i + 1 == terrain_mesh.dist_count
      ? range
      : ForwardViewGeometry::MIN_TERRAIN_DIST + double(i) * dist_step;
    terrain_mesh.xs[i] = ahead;

    const double half_width =
      ForwardViewGeometry::LateralHalfWidthAtDistance(
        std::max(ahead, ForwardViewGeometry::MIN_TERRAIN_DIST), aspect);
    const GeoPoint row_center =
      GeoVector{ahead, track}.EndPoint(origin);

    terrain_mesh.ys[i].resize(terrain_mesh.lateral_count);
    terrain_mesh.raw_heights[i].resize(terrain_mesh.lateral_count);
    terrain_mesh.terrain_heights[i].resize(terrain_mesh.lateral_count);

    for (unsigned j = 0; j < terrain_mesh.lateral_count; ++j) {
      const double lateral =
        (double(j) / double(terrain_mesh.lateral_count - 1) - 0.5)
        * 2. * half_width;
      terrain_mesh.ys[i][j] = lateral;

      const Angle lat_bearing = lateral >= 0.
        ? lateral_bearing
        : lateral_bearing + Angle::HalfCircle();
      const GeoPoint p =
        GeoVector{std::abs(lateral), lat_bearing}.EndPoint(row_center);

      const TerrainHeight h = ForwardViewTerrain::MeshHeightAt(map, p);
      terrain_mesh.terrain_heights[i][j] = h;
      terrain_mesh.raw_heights[i][j] =
        h.IsSpecial() ? 0. : h.GetValue();
    }
  }

  terrain_mesh.valid = true;

  terrain_mesh.gpu_triangle_count = 0;
}

static ForwardViewTerrainShader::MeshVertex
MakeGpuMeshVertex(double x, double y_geo, double msl,
                  float material,
                  float base_r, float base_g, float base_b,
                  double nx, double ny, double nz,
                  float shadow_cap,
                  float topo_u, float topo_v) noexcept
{
  return {
    float(x), float(y_geo), float(msl),
    material,
    base_r, base_g, base_b,
    float(nx), float(ny), float(nz),
    shadow_cap,
    topo_u, topo_v,
  };
}

static void
AppendGpuTriangle(std::vector<ForwardViewTerrainShader::MeshVertex> &vertices,
                  ForwardViewTerrainShader::MeshVertex a,
                  ForwardViewTerrainShader::MeshVertex b,
                  ForwardViewTerrainShader::MeshVertex c) noexcept
{
  vertices.push_back(a);
  vertices.push_back(b);
  vertices.push_back(c);
}

static void
AppendGpuQuad(std::vector<ForwardViewTerrainShader::MeshVertex> &vertices,
              ForwardViewTerrainShader::MeshVertex a,
              ForwardViewTerrainShader::MeshVertex b,
              ForwardViewTerrainShader::MeshVertex c,
              ForwardViewTerrainShader::MeshVertex d) noexcept
{
  AppendGpuTriangle(vertices, a, b, c);
  AppendGpuTriangle(vertices, a, c, d);
}

void
ForwardViewRenderer::BuildTerrainGpuMesh(glm::vec3 sun_dir, bool sun_active,
                                         bool cast_shadows) const noexcept
{
  terrain_mesh.gpu_triangle_count = 0;
  if (!terrain_mesh.valid)
    return;

  const double ref_alt = terrain_mesh.vertical_ref_alt;
  const double vertical_ref = ref_alt;

  const auto &xs = terrain_mesh.xs;
  const auto &ys = terrain_mesh.ys;
  const auto &raw_heights = terrain_mesh.raw_heights;
  const auto &terrain_heights = terrain_mesh.terrain_heights;
  const unsigned dist_count = terrain_mesh.dist_count;
  const unsigned lateral_count = terrain_mesh.lateral_count;
  const double shadow_step = terrain_mesh.dem_cell_spacing;

  std::vector<std::vector<double>> heights(dist_count);
  for (unsigned i = 0; i < dist_count; ++i) {
    heights[i].resize(lateral_count);
    for (unsigned j = 0; j < lateral_count; ++j) {
      const TerrainHeight &h = terrain_heights[i][j];
      heights[i][j] = ForwardViewTerrain::MeshElevation(
        raw_heights[i][j], ref_alt, vertical_ref,
        xs[i], ys[i][j], h);
    }
  }

  std::vector<ForwardViewTerrainShader::MeshVertex> vertices;
  vertices.reserve(size_t(dist_count - 1) * size_t(lateral_count - 1) * 6);

  const float shadow_cap_value =
    float(ForwardViewGeometry::CAST_SHADOW_ILLUM);
  const unsigned ramp_index = terrain_settings.ramp;

  for (unsigned i = 0; i + 1 < dist_count; ++i) {
    const double x0 = xs[i];
    const double x1 = xs[i + 1];
    if (x1 <= x0 || x0 < 0. ||
        x1 < ForwardViewGeometry::MIN_TERRAIN_DIST)
      continue;

    for (unsigned j = 0; j + 1 < lateral_count; ++j) {
      const TerrainHeight &h00 = terrain_heights[i][j];
      const TerrainHeight &h10 = terrain_heights[i + 1][j];
      const TerrainHeight &h01 = terrain_heights[i][j + 1];
      const TerrainHeight &h11 = terrain_heights[i + 1][j + 1];
      if (h00.IsInvalid() || h10.IsInvalid() ||
          h01.IsInvalid() || h11.IsInvalid())
        continue;

      double nx00, ny00, nz00;
      double nx10, ny10, nz10;
      double nx01, ny01, nz01;
      double nx11, ny11, nz11;
      ComputeVertexNormal(i, j, dist_count, lateral_count,
                          xs, ys, heights, nx00, ny00, nz00);
      ComputeVertexNormal(i + 1, j, dist_count, lateral_count,
                          xs, ys, heights, nx10, ny10, nz10);
      ComputeVertexNormal(i, j + 1, dist_count, lateral_count,
                          xs, ys, heights, nx01, ny01, nz01);
      ComputeVertexNormal(i + 1, j + 1, dist_count, lateral_count,
                          xs, ys, heights, nx11, ny11, nz11);

      auto material_of = [](const TerrainHeight &h) -> float {
        return h.IsWater() ? 1.f : 0.f;
      };

      auto msl_of = [](const TerrainHeight &h, double raw) -> double {
        return h.IsWater() ? 0. : raw;
      };

      auto base_of = [ramp_index](const TerrainHeight &h, double raw)
        -> std::array<float, 3> {
        const Color c = LookupMapTerrainColor(h, raw, ramp_index);
        return {
          float(c.Red()) / 255.f,
          float(c.Green()) / 255.f,
          float(c.Blue()) / 255.f,
        };
      };

      auto shadow_of = [&](double px, double py, double pz) -> float {
        if (!cast_shadows || !sun_active)
          return 0.f;

        if (ForwardViewTerrain::PointCastShadow(
              px, py, pz,
              sun_dir.x, sun_dir.y, sun_dir.z,
              xs, ys, heights, dist_count, lateral_count,
              shadow_step))
          return shadow_cap_value;

        return 0.f;
      };

      const bool use_topo_uv =
        topography_enabled && topo_texture.IsValid();
      auto uv_of = [&](double px, double py) noexcept {
        if (!use_topo_uv)
          return ForwardViewTopographyTexture::UV{0.f, 0.f};
        return topo_texture.ComputeUV(px, py);
      };

      const ForwardViewTopographyTexture::UV uv00 = uv_of(x0, ys[i][j]);
      const ForwardViewTopographyTexture::UV uv10 =
        uv_of(x1, ys[i + 1][j]);
      const ForwardViewTopographyTexture::UV uv01 =
        uv_of(x0, ys[i][j + 1]);
      const ForwardViewTopographyTexture::UV uv11 =
        uv_of(x1, ys[i + 1][j + 1]);

      const auto rgb00 = base_of(h00, raw_heights[i][j]);
      const auto rgb10 = base_of(h10, raw_heights[i + 1][j]);
      const auto rgb01 = base_of(h01, raw_heights[i][j + 1]);
      const auto rgb11 = base_of(h11, raw_heights[i + 1][j + 1]);

      const ForwardViewTerrainShader::MeshVertex v00 = MakeGpuMeshVertex(
        x0, ys[i][j], msl_of(h00, raw_heights[i][j]),
        material_of(h00), rgb00[0], rgb00[1], rgb00[2],
        nx00, ny00, nz00,
        shadow_of(x0, ys[i][j], heights[i][j]),
        uv00.u, uv00.v);
      const ForwardViewTerrainShader::MeshVertex v10 = MakeGpuMeshVertex(
        x1, ys[i + 1][j], msl_of(h10, raw_heights[i + 1][j]),
        material_of(h10), rgb10[0], rgb10[1], rgb10[2],
        nx10, ny10, nz10,
        shadow_of(x1, ys[i + 1][j], heights[i + 1][j]),
        uv10.u, uv10.v);
      const ForwardViewTerrainShader::MeshVertex v01 = MakeGpuMeshVertex(
        x0, ys[i][j + 1], msl_of(h01, raw_heights[i][j + 1]),
        material_of(h01), rgb01[0], rgb01[1], rgb01[2],
        nx01, ny01, nz01,
        shadow_of(x0, ys[i][j + 1], heights[i][j + 1]),
        uv01.u, uv01.v);
      const ForwardViewTerrainShader::MeshVertex v11 = MakeGpuMeshVertex(
        x1, ys[i + 1][j + 1], msl_of(h11, raw_heights[i + 1][j + 1]),
        material_of(h11), rgb11[0], rgb11[1], rgb11[2],
        nx11, ny11, nz11,
        shadow_of(x1, ys[i + 1][j + 1], heights[i + 1][j + 1]),
        uv11.u, uv11.v);

      AppendGpuQuad(vertices, v00, v10, v11, v01);
    }
  }

  if (vertices.empty())
    return;

  if (!ForwardViewTerrainShader::EnsureInitialised())
    return;

  const unsigned n_triangles = unsigned(vertices.size() / 3);
  ForwardViewTerrainShader::UploadMesh(vertices.data(), n_triangles);
  terrain_mesh.gpu_triangle_count = n_triangles;
  terrain_mesh.gpu_ramp = ramp_index;
}

void
ForwardViewRenderer::DrawTerrainGpu(double ref_alt, float eye_z,
                                    glm::vec3 sun_dir, bool sun_active,
                                    const glm::mat4 &projection,
                                    const glm::mat4 &view) const noexcept
{
  if (terrain_mesh.gpu_triangle_count == 0)
    return;

  double scroll_x = 0., scroll_y = 0.;
  if (terrain_mesh.anchor.IsValid() && start.IsValid()) {
    const GeoVector rel(terrain_mesh.anchor, start);
    const Angle rel_bearing = rel.bearing - terrain_mesh.track;
    scroll_x = rel.distance * rel_bearing.cos();
    scroll_y = rel.distance * rel_bearing.sin();
  }

  if (!ForwardViewTerrainShader::EnsureInitialised())
    return;

  const bool topo_active =
    topography_enabled && topo_texture.IsValid();
  if (topo_active) {
    glActiveTexture(GL_TEXTURE0);
    topo_texture.BindTexture();
  }

  const float ramp_weight = topo_active
    ? ForwardViewGeometry::TERRAIN_RAMP_TOPO_WEIGHT
    : 1.f;

  ForwardViewTerrainShader::DrawParams params{
    projection,
    view,
    glm::vec2(float(scroll_x), float(scroll_y)),
    float(ref_alt),
    float(terrain_mesh.vertical_ref_alt),
    eye_z,
    sun_dir,
    sun_active,
    topo_active,
    ramp_weight,
  };
  ForwardViewTerrainShader::Draw(params);
}

void
ForwardViewRenderer::DrawTerrainMesh(std::vector<Vertex3D> &opaque_vertices,
                                     std::vector<Vertex3D> &fade_vertices,
                                     double ref_alt, double eye_z,
                                     const SunLight &sun) const noexcept
{
  if (!terrain_mesh.valid)
    return;

  const GeoVector rel(terrain_mesh.anchor, start);
  const Angle rel_bearing = rel.bearing - terrain_mesh.track;
  const double dx = rel.distance * rel_bearing.cos();
  const double dy = rel.distance * rel_bearing.sin();

  const auto &xs = terrain_mesh.xs;
  const auto &ys = terrain_mesh.ys;
  const auto &raw_heights = terrain_mesh.raw_heights;
  const auto &terrain_heights = terrain_mesh.terrain_heights;
  const unsigned dist_count = terrain_mesh.dist_count;
  const unsigned lateral_count = terrain_mesh.lateral_count;
  const double shadow_step = terrain_mesh.dem_cell_spacing;
  const double vertical_ref = terrain_mesh.vertical_ref_alt;
  const float ramp_weight =
    topography_enabled && topo_texture.IsValid()
      ? ForwardViewGeometry::TERRAIN_RAMP_TOPO_WEIGHT
      : 1.f;
  const unsigned ramp_index = terrain_mesh.ramp;

  std::vector<std::vector<double>> heights(dist_count);
  for (unsigned i = 0; i < dist_count; ++i) {
    heights[i].resize(lateral_count);
    const double x = xs[i] - dx;
    for (unsigned j = 0; j < lateral_count; ++j) {
      const TerrainHeight &h = terrain_heights[i][j];
      heights[i][j] = ForwardViewTerrain::MeshElevation(
        raw_heights[i][j], ref_alt, vertical_ref,
        x, ys[i][j] - dy, h);
    }
  }

  for (unsigned i = 0; i + 1 < dist_count; ++i) {
    const double x0 = xs[i] - dx;
    const double x1 = xs[i + 1] - dx;
    if (x1 <= x0 || x0 < 0. || x1 < ForwardViewGeometry::MIN_TERRAIN_DIST)
      continue;

    for (unsigned j = 0; j + 1 < lateral_count; ++j) {
      const TerrainHeight &h00 = terrain_heights[i][j];
      const TerrainHeight &h10 = terrain_heights[i + 1][j];
      const TerrainHeight &h01 = terrain_heights[i][j + 1];
      const TerrainHeight &h11 = terrain_heights[i + 1][j + 1];
      if (h00.IsInvalid() || h10.IsInvalid() ||
          h01.IsInvalid() || h11.IsInvalid())
        continue;

      const double y00 = ys[i][j] - dy;
      const double y10 = ys[i + 1][j] - dy;
      const double y11 = ys[i + 1][j + 1] - dy;
      const double y01 = ys[i][j + 1] - dy;

      double nx00, ny00, nz00;
      double nx10, ny10, nz10;
      double nx01, ny01, nz01;
      double nx11, ny11, nz11;
      ComputeVertexNormal(i, j, dist_count, lateral_count,
                          xs, ys, heights, nx00, ny00, nz00);
      ComputeVertexNormal(i + 1, j, dist_count, lateral_count,
                          xs, ys, heights, nx10, ny10, nz10);
      ComputeVertexNormal(i, j + 1, dist_count, lateral_count,
                          xs, ys, heights, nx01, ny01, nz01);
      ComputeVertexNormal(i + 1, j + 1, dist_count, lateral_count,
                          xs, ys, heights, nx11, ny11, nz11);

      const Color c00 = ShadedTerrainColor(terrain_heights[i][j],
                                           raw_heights[i][j],
                                           TerrainIllumination(nx00, ny00, nz00,
                                                               x0, y00,
                                                               heights[i][j],
                                                               dx, dy,
                                                               sun, xs, ys,
                                                               heights,
                                                               dist_count,
                                                               lateral_count,
                                                               shadow_step),
                                           x0, y00, heights[i][j],
                                           eye_z, ref_alt, ramp_index,
                                           ramp_weight);
      const Color c10 = ShadedTerrainColor(terrain_heights[i + 1][j],
                                           raw_heights[i + 1][j],
                                           TerrainIllumination(nx10, ny10, nz10,
                                                               x1, y10,
                                                               heights[i + 1][j],
                                                               dx, dy,
                                                               sun, xs, ys,
                                                               heights,
                                                               dist_count,
                                                               lateral_count,
                                                               shadow_step),
                                           x1, y10, heights[i + 1][j],
                                           eye_z, ref_alt, ramp_index,
                                           ramp_weight);
      const Color c01 = ShadedTerrainColor(terrain_heights[i][j + 1],
                                           raw_heights[i][j + 1],
                                           TerrainIllumination(nx01, ny01, nz01,
                                                               x0, y01,
                                                               heights[i][j + 1],
                                                               dx, dy,
                                                               sun, xs, ys,
                                                               heights,
                                                               dist_count,
                                                               lateral_count,
                                                               shadow_step),
                                           x0, y01, heights[i][j + 1],
                                           eye_z, ref_alt, ramp_index,
                                           ramp_weight);
      const Color c11 = ShadedTerrainColor(terrain_heights[i + 1][j + 1],
                                           raw_heights[i + 1][j + 1],
                                           TerrainIllumination(nx11, ny11, nz11,
                                                               x1, y11,
                                                               heights[i + 1][j + 1],
                                                               dx, dy,
                                                               sun, xs, ys,
                                                               heights,
                                                               dist_count,
                                                               lateral_count,
                                                               shadow_step),
                                           x1, y11, heights[i + 1][j + 1],
                                           eye_z, ref_alt, ramp_index,
                                           ramp_weight);

      if (wireframe) {
        AppendLineSplit(opaque_vertices, fade_vertices,
                          x0, y00, heights[i][j],
                          x1, y10, heights[i + 1][j], c00, c10);
        AppendLineSplit(opaque_vertices, fade_vertices,
                          x1, y10, heights[i + 1][j], x1, y11,
                          heights[i + 1][j + 1], c10, c11);
        AppendLineSplit(opaque_vertices, fade_vertices,
                          x1, y11, heights[i + 1][j + 1], x0, y01,
                          heights[i][j + 1], c11, c01);
        AppendLineSplit(opaque_vertices, fade_vertices,
                          x0, y01, heights[i][j + 1], x0, y00,
                          heights[i][j], c01, c00);
      } else {
        AppendQuadSplit(opaque_vertices, fade_vertices,
                        MakeDisplayVertex(x0, y00, heights[i][j], c00),
                        MakeDisplayVertex(x1, y10, heights[i + 1][j], c10),
                        MakeDisplayVertex(x1, y11, heights[i + 1][j + 1], c11),
                        MakeDisplayVertex(x0, y01, heights[i][j + 1], c01));
      }
    }
  }
}

void
ForwardViewRenderer::DrawTerrainGrid(std::vector<Vertex3D> &vertices,
                                     double ref_alt, double eye_z) const noexcept
{
  if (!terrain_mesh.valid)
    return;

  const GeoVector rel(terrain_mesh.anchor, start);
  const Angle rel_bearing = rel.bearing - terrain_mesh.track;
  const double dx = rel.distance * rel_bearing.cos();
  const double dy = rel.distance * rel_bearing.sin();

  const auto &xs = terrain_mesh.xs;
  const auto &ys = terrain_mesh.ys;
  const auto &raw_heights = terrain_mesh.raw_heights;
  const auto &terrain_heights = terrain_mesh.terrain_heights;
  const unsigned dist_count = terrain_mesh.dist_count;
  const unsigned lateral_count = terrain_mesh.lateral_count;
  const double vertical_ref = terrain_mesh.vertical_ref_alt;
  const double lift = ForwardViewGeometry::TERRAIN_GRID_OFFSET;

  std::vector<std::vector<double>> heights(dist_count);
  for (unsigned i = 0; i < dist_count; ++i) {
    heights[i].resize(lateral_count);
    const double x = xs[i] - dx;
    for (unsigned j = 0; j < lateral_count; ++j) {
      const TerrainHeight &h = terrain_heights[i][j];
      heights[i][j] = ForwardViewTerrain::MeshElevation(
        raw_heights[i][j], ref_alt, vertical_ref,
        x, ys[i][j] - dy, h);
    }
  }

  const auto grid_z = [&](unsigned i, unsigned j) -> double {
    return heights[i][j] + lift;
  };

  const auto skip = [&](unsigned i, unsigned j) -> bool {
    return terrain_heights[i][j].IsInvalid();
  };

  const auto line = [&](double x1, double y1, double z1,
                        double x2, double y2, double z2) {
    const double mx = (x1 + x2) * 0.5;
    const double my = (y1 + y2) * 0.5;
    const double mz = (z1 + z2) * 0.5;
    const Color color = GridLineColor(mx, my, mz, eye_z, ref_alt);
    if (color.Alpha() == 0)
      return;

    AppendLine(vertices, x1, y1, z1, x2, y2, z2, color);
  };

  const double range = terrain_mesh.range;

  for (unsigned i = 0; i + 1 < dist_count; ++i) {
    const double x = xs[i] - dx;
    if (x < ForwardViewGeometry::MIN_TERRAIN_DIST ||
        x > range * 0.88)
      continue;

    for (unsigned j = 0; j + 1 < lateral_count; ++j) {
      if (skip(i, j) || skip(i, j + 1))
        continue;

      line(x, ys[i][j] - dy, grid_z(i, j),
           x, ys[i][j + 1] - dy, grid_z(i, j + 1));
    }
  }
}

static void
DrawVertices(const std::vector<ForwardViewRenderer::Vertex3D> &vertices,
             GLenum mode) noexcept
{
  if (vertices.empty())
    return;

  OpenGL::solid_shader->Use();

  glEnableVertexAttribArray(OpenGL::Attribute::POSITION);
  glEnableVertexAttribArray(OpenGL::Attribute::COLOR);

  using Vertex3D = ForwardViewRenderer::Vertex3D;
  glVertexAttribPointer(OpenGL::Attribute::POSITION, 3, GL_FLOAT, GL_FALSE,
                        sizeof(Vertex3D),
                        &vertices.front().x);
  glVertexAttribPointer(OpenGL::Attribute::COLOR, 4, GL_FLOAT, GL_FALSE,
                        sizeof(Vertex3D),
                        &vertices.front().r);

  const GLsizei count = GLsizei(std::min(vertices.size(),
                                         size_t(std::numeric_limits<GLsizei>::max())));
  glDrawArrays(mode, 0, count);

  glDisableVertexAttribArray(OpenGL::Attribute::COLOR);
  glDisableVertexAttribArray(OpenGL::Attribute::POSITION);
}

static void
DrawSky(double eye_z, double ref_alt, float aspect,
        Color horizon_color, Color zenith_color) noexcept
{
  const double hfov_half =
    ForwardViewGeometry::HorizontalFovRadians(aspect) * 0.5 * 1.15;
  const double sky_bottom =
    ForwardViewGeometry::EyeHorizonElevation(ref_alt, eye_z) -
    ForwardViewGeometry::SKY_BELOW_HORIZON;
  constexpr unsigned elev_steps = 20;
  constexpr unsigned az_steps = 32;

  std::vector<ForwardViewRenderer::Vertex3D> vertices;

  auto sky_point = [&](double elev, double az) {
    const double ce = std::cos(elev);
    return std::make_tuple(SKY_RADIUS * ce * std::cos(az),
                           SKY_RADIUS * ce * std::sin(az),
                           eye_z + SKY_RADIUS * std::sin(elev));
  };

  for (unsigned e = 0; e < elev_steps; ++e) {
    const double t0 = double(e) / double(elev_steps);
    const double t1 = double(e + 1) / double(elev_steps);
    const double el0 = sky_bottom + t0 * (M_PI_2 - sky_bottom);
    const double el1 = sky_bottom + t1 * (M_PI_2 - sky_bottom);
    const Color c0 = LerpColor(horizon_color, zenith_color, t0);
    const Color c1 = LerpColor(horizon_color, zenith_color, t1);

    for (unsigned a = 0; a < az_steps; ++a) {
      const double az0 = -hfov_half + (2. * hfov_half * a) / az_steps;
      const double az1 = -hfov_half + (2. * hfov_half * (a + 1)) / az_steps;

      const auto [x00, y00, z00] = sky_point(el0, az0);
      const auto [x10, y10, z10] = sky_point(el0, az1);
      const auto [x01, y01, z01] = sky_point(el1, az0);
      const auto [x11, y11, z11] = sky_point(el1, az1);

      AppendQuad(vertices,
                 MakeVertex(x00, y00, z00, c0),
                 MakeVertex(x10, y10, z10, c0),
                 MakeVertex(x11, y11, z11, c1),
                 MakeVertex(x01, y01, z01, c1));
    }
  }

  glDepthMask(GL_FALSE);
  DrawVertices(vertices, GL_TRIANGLES);
  glDepthMask(GL_TRUE);
}

static void
DrawSunDisc(const SunLight &sun) noexcept
{
  if (!sun.visible)
    return;

  const glm::vec3 dir(float(sun.dir_x),
                      float(ForwardViewGeometry::DisplayLateral(sun.dir_y)),
                      float(sun.dir_z));
  glm::vec3 tangent = glm::cross(dir, glm::vec3(0.f, 0.f, 1.f));
  if (glm::dot(tangent, tangent) < 1e-4f)
    tangent = glm::vec3(0.f, 1.f, 0.f);
  tangent = glm::normalize(tangent);
  const glm::vec3 bitangent = glm::normalize(glm::cross(dir, tangent));

  const glm::vec3 center = dir * float(SUN_DISTANCE);

  std::vector<ForwardViewRenderer::Vertex3D> vertices;
  constexpr unsigned n_segments = 24;
  const Color edge = Color(255, 220, 80);

  for (unsigned i = 0; i < n_segments; ++i) {
    const double a0 = 2. * M_PI * double(i) / double(n_segments);
    const double a1 = 2. * M_PI * double(i + 1) / double(n_segments);
    const glm::vec3 p0 = center +
      (tangent * float(std::cos(a0)) + bitangent * float(std::sin(a0))) *
      float(SUN_RADIUS);
    const glm::vec3 p1 = center +
      (tangent * float(std::cos(a1)) + bitangent * float(std::sin(a1))) *
      float(SUN_RADIUS);

    AppendTriangle(vertices,
                   MakeVertex(center.x, center.y, center.z, COLOR_WHITE),
                   MakeVertex(p0.x, p0.y, p0.z, edge),
                   MakeVertex(p1.x, p1.y, p1.z, edge));
  }

  DrawVertices(vertices, GL_TRIANGLES);
}

void
ForwardViewRenderer::PaintOpenGL(Canvas &canvas, const PixelRect &rc,
                                 double ref_alt) noexcept
{
  if (rc.GetWidth() < 2u || rc.GetHeight() < 2u) {
    canvas.Clear(SkyZenithColor(look.sky_color));
    return;
  }

  canvas.Clear(SkyZenithColor(look.sky_color));

  const float aspect = std::max(0.01f,
    float(rc.GetWidth()) / float(rc.GetHeight()));

  double local_ground_z = 0.;
  if (terrain != nullptr && start.IsValid()) {
    RasterTerrain::Lease map(*terrain);
    const TerrainHeight h = ForwardViewTerrain::HeightAt(map, start);
    if (!h.IsInvalid()) {
      const double vertical_ref = terrain_mesh.valid
        ? terrain_mesh.vertical_ref_alt
        : ref_alt;
      local_ground_z = ForwardViewTerrain::MeshElevation(
        h.GetValue(), ref_alt, vertical_ref, 0., 0., h);
    }
  }

  const float eye_z = float(ForwardViewGeometry::CameraEyeZ(local_ground_z));

  const glm::mat4 projection = glm::perspective(
    glm::radians(float(ForwardViewGeometry::CAMERA_FOV_DEG)),
    aspect, ForwardViewGeometry::CAMERA_NEAR_CLIP,
    ForwardViewGeometry::CAMERA_FAR_CLIP);

  const double pitch =
    Angle::Degrees(ForwardViewGeometry::CAMERA_PITCH_DEG).Radians();
  const double look_ahead = 12000.;
  const glm::vec3 eye(0.f, 0.f, eye_z);
  const glm::vec3 center(float(look_ahead * std::cos(pitch)), 0.f,
                         float(eye_z - look_ahead * std::sin(pitch)));
  const glm::mat4 view = glm::lookAt(eye, center, glm::vec3(0.f, 0.f, 1.f));

  std::vector<ForwardViewTopography::Sprite> topo_sprites;

  const Color horizon_color = SkyHorizonColor(look.sky_color);
  const Color zenith_color = SkyZenithColor(look.sky_color);

  OpenGL::solid_shader->Use();
  glUniformMatrix4fv(OpenGL::solid_projection, 1, GL_FALSE,
                     glm::value_ptr(projection));
  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(view));
  glUniform2f(OpenGL::solid_translate, 0.f, 0.f);

  const GLEnable<GL_DEPTH_TEST> depth;
  glClear(GL_DEPTH_BUFFER_BIT);

  DrawSky(eye_z, ref_alt, aspect, horizon_color, zenith_color);

  const SunLight sun = GetForwardViewSunLight(Calculated(), Basic(),
                                              utc_offset, forward.bearing);
  if (ForwardViewGeometry::SLOPE_SHADING_ENABLED)
    DrawSunDisc(sun);

  std::vector<Vertex3D> terrain_opaque;
  std::vector<Vertex3D> terrain_fade;
  bool terrain_rebuilt = false;
  EnsureTerrainMeshCache(ref_alt, aspect, &terrain_rebuilt);

  bool topo_rebuilt = false;
  if (topography != nullptr && topography_enabled && terrain_mesh.valid)
    topo_texture.Ensure(terrain_mesh.anchor, terrain_mesh.track,
                        terrain_mesh.range, terrain_mesh.aspect,
                        topography->GetSerial(), &topo_rebuilt);

  if (terrain_mesh.valid &&
      (terrain_rebuilt || topo_rebuilt ||
       terrain_mesh.gpu_triangle_count == 0 ||
       terrain_mesh.gpu_ramp != terrain_settings.ramp)) {
    const SunLight sun_mesh = GetForwardViewSunLight(Calculated(), Basic(),
                                                     utc_offset,
                                                     forward.bearing);
    BuildTerrainGpuMesh(glm::vec3(float(sun_mesh.dir_x),
                                  float(sun_mesh.dir_y),
                                  float(sun_mesh.dir_z)),
                        sun_mesh.active,
                        ForwardViewGeometry::CAST_SHADOWS_ENABLED);
  }

  if (!wireframe && terrain_mesh.gpu_triangle_count > 0) {
    DrawTerrainGpu(ref_alt, eye_z,
                   glm::vec3(float(sun.dir_x), float(sun.dir_y),
                             float(sun.dir_z)),
                   sun.active, projection, view);
    OpenGL::solid_shader->Use();
    glUniformMatrix4fv(OpenGL::solid_projection, 1, GL_FALSE,
                       glm::value_ptr(projection));
    glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                       glm::value_ptr(view));
    glUniform2f(OpenGL::solid_translate, 0.f, 0.f);
    glDepthMask(GL_TRUE);
  } else {
    DrawTerrainMesh(terrain_opaque, terrain_fade, ref_alt, eye_z, sun);
    if (wireframe)
      glLineWidth(float(Layout::ScaleFinePenWidth(1)));
    DrawVertices(terrain_opaque, wireframe ? GL_LINES : GL_TRIANGLES);
    if (!terrain_fade.empty()) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDepthMask(GL_FALSE);
      DrawVertices(terrain_fade, wireframe ? GL_LINES : GL_TRIANGLES);
      glDepthMask(GL_TRUE);
      glDisable(GL_BLEND);
    }
  }

  if (ForwardViewGeometry::TERRAIN_GRID_ENABLED && !wireframe) {
    std::vector<Vertex3D> grid_vertices;
    DrawTerrainGrid(grid_vertices, ref_alt, eye_z);
    if (!grid_vertices.empty()) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glLineWidth(float(Layout::ScaleFinePenWidth(1)));
      DrawVertices(grid_vertices, GL_LINES);
      glDisable(GL_BLEND);
    }
  }

  glLineWidth(1.f);

  if (topography != nullptr && topography_enabled)
    DrawTopography(rc, ref_alt, topo_sprites);

#ifdef HAVE_HTTP
  if (!wireframe && terrain_mesh.valid && start.IsValid())
    DrawXCThermVolumes(rc, ref_alt, aspect);
#endif

  if (!wireframe && airspace_enabled && airspaces != nullptr &&
      terrain_mesh.valid && start.IsValid())
    DrawAirspaces(rc, ref_alt, aspect);

  if (Basic().NavAltitudeAvailable() && glide_polar.IsValid()) {
    std::vector<Vertex3D> line_vertices;
    const MacCready mc(glide_settings, glide_polar);
    const GlideState task(forward, 0, ref_alt,
                          Calculated().GetWindOrZero());
    const GlideResult result = mc.SolveStraight(task);
    if (result.IsOk()) {
      const double x = result.vector.distance;
      const double z = std::max(-ref_alt,
                                result.GetArrivalAltitude() - ref_alt);
      AppendLine(line_vertices,
                 0, 0, 0,
                 x, 0, z,
                 COLOR_BLUE.WithAlpha(0xff));
      DrawVertices(line_vertices, GL_LINES);
    }
  }

  const TaskStats &task_stats = Calculated().task_stats;
  if (task_stats.task_valid && !task_stats.task_finished) {
    const ElementStat &leg = task_stats.current_leg;
    if (leg.location_remaining.IsValid()) {
      const GeoVector rel(start, leg.location_remaining);
      const Angle rel_bearing = rel.bearing - forward.bearing;
      const double x = rel.distance * rel_bearing.cos();
      const double y = rel.distance * rel_bearing.sin();

      std::vector<Vertex3D> task_vertices;
      AppendLine(task_vertices,
                 0, 0, 0,
                 x, y, 0,
                 COLOR_YELLOW.WithAlpha(0xff));
      DrawVertices(task_vertices, GL_LINES);
    }
  }

  std::vector<Vertex3D> aircraft_vertices;
  const double wing = 200.;
  AppendLine(aircraft_vertices,
             -wing, 0, 0,
             wing * 2, 0, 0,
             COLOR_WHITE.WithAlpha(0xff));
  AppendLine(aircraft_vertices,
             0, -wing * 0.4, 0,
             0, wing * 0.4, 0,
             COLOR_WHITE.WithAlpha(0xff));
  DrawVertices(aircraft_vertices, GL_LINES);

  if (!topo_sprites.empty())
    DrawTopographySprites(rc, ref_alt, topo_sprites, projection, view);

  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(glm::mat4(1)));
  glUniformMatrix4fv(OpenGL::solid_projection, 1, GL_FALSE,
                     glm::value_ptr(OpenGL::projection_matrix));
}

const MaskedIcon &
ForwardViewRenderer::GetTopoIcon(ResourceId icon, ResourceId big_icon,
                               ResourceId ultra_icon) const noexcept
{
  for (const auto &entry : topo_icon_cache) {
    if (entry.icon == icon && entry.big_icon == big_icon &&
        entry.ultra_icon == ultra_icon)
      return entry.masked;
  }

  TopoIconCacheEntry entry{
    icon, big_icon, ultra_icon, MaskedIcon{},
  };
  entry.masked.LoadResource(icon, big_icon, ultra_icon);
  topo_icon_cache.push_back(std::move(entry));
  return topo_icon_cache.back().masked;
}

void
ForwardViewRenderer::DrawTopographySprites(
  const PixelRect &rc, [[maybe_unused]] double ref_alt,
  const std::vector<ForwardViewTopography::Sprite> &sprites,
  const glm::mat4 &projection, const glm::mat4 &view) const noexcept
{
  const glm::mat4 mvp = projection * view;
  const glm::mat4 inv_view = glm::inverse(view);
  const glm::vec3 right = glm::normalize(glm::vec3(inv_view[0]));
  const glm::vec3 up = glm::normalize(glm::vec3(inv_view[1]));
  const float vfov = glm::radians(float(ForwardViewGeometry::CAMERA_FOV_DEG));
  const float screen_h = float(std::max(1u, rc.GetHeight()));

  const GLEnable<GL_DEPTH_TEST> depth;
  glDepthMask(GL_TRUE);

  for (const auto &sprite : sprites) {
    const glm::vec4 clip = mvp * glm::vec4(
      sprite.x,
      ForwardViewGeometry::DisplayLateral(sprite.y),
      sprite.z, 1.f);
    if (clip.w <= 0.f)
      continue;

    const float inv_w = 1.f / clip.w;
    const float ndc_x = clip.x * inv_w;
    const float ndc_y = clip.y * inv_w;
    const float ndc_z = clip.z * inv_w;
    if (ndc_x < -1.2f || ndc_x > 1.2f || ndc_y < -1.2f || ndc_y > 1.2f ||
        ndc_z < -1.f || ndc_z > 1.f)
      continue;

    const MaskedIcon &icon = GetTopoIcon(sprite.icon, sprite.big_icon,
                                         sprite.ultra_icon);
    if (!icon.IsDefined())
      continue;

    const PixelSize &icon_size = icon.GetSize();
    if (icon_size.height == 0)
      continue;

    const glm::vec3 center(
      sprite.x,
      float(ForwardViewGeometry::DisplayLateral(sprite.y)),
      sprite.z);
    const glm::vec3 cam(0.f, 0.f, 0.f);
    const float dist = glm::length(center - cam);
    const float half_h = dist * std::tan(vfov * 0.5f) *
      float(icon_size.height) / screen_h;
    const float half_w = half_h * float(icon_size.width) /
      float(icon_size.height);

    const glm::vec3 hw = right * half_w;
    const glm::vec3 hh = up * half_h;

    MaskedIcon::BillboardCorner corners[4] = {
      {center.x - hw.x - hh.x, center.y - hw.y - hh.y, center.z - hw.z - hh.z},
      {center.x + hw.x - hh.x, center.y + hw.y - hh.y, center.z + hw.z - hh.z},
      {center.x - hw.x + hh.x, center.y - hw.y + hh.y, center.z - hw.z + hh.z},
      {center.x + hw.x + hh.x, center.y + hw.y + hh.y, center.z + hw.z + hh.z},
    };

    icon.DrawBillboard(glm::value_ptr(mvp), corners);
  }
}

void
ForwardViewRenderer::DrawTopography(const PixelRect &rc, double ref_alt,
                                    std::vector<ForwardViewTopography::Sprite> &sprites) noexcept
{
  if (terrain == nullptr || topography == nullptr || !forward.IsValid())
    return;

  const double vertical_ref = terrain_mesh.valid
    ? terrain_mesh.vertical_ref_alt
    : ref_alt;

  const float aspect = float(rc.GetWidth()) / float(std::max(1u, rc.GetHeight()));
  const double range = forward.distance;
  const Angle track = forward.bearing;
  const unsigned store_serial = topography->GetSerial();

  const GeoPoint build_origin = terrain_mesh.valid
    ? terrain_mesh.anchor
    : start;

  auto cache_stale = [&]() noexcept {
    if (!topo_sprite_cache.valid)
      return true;

    if (topo_sprite_cache.store_serial != store_serial ||
        !track.CompareRoughly(topo_sprite_cache.track) ||
        topo_sprite_cache.range != range ||
        topo_sprite_cache.aspect != aspect ||
        topo_sprite_cache.used_texture_path != topo_texture.IsValid())
      return true;

    const GeoVector rel(topo_sprite_cache.anchor, build_origin);
    const Angle rel_bearing = rel.bearing - track;
    const double along = rel.distance * rel_bearing.cos();
    return along < 0. || along >= range * 0.7;
  };

  if (cache_stale()) {
    topo_sprite_cache.sprites.clear();
    topo_sprite_cache.fallback_lines.clear();
    topo_sprite_cache.water_fills.clear();
    {
      RasterTerrain::Lease map(*terrain);
      ForwardViewTopography::BuildSprites(topo_sprite_cache.sprites,
                                          *topography, map, build_origin,
                                          forward, ref_alt, vertical_ref, rc);
      if (topo_texture.IsValid()) {
        ForwardViewTopography::BuildWaterFills(topo_sprite_cache.water_fills,
                                               *topography, map, build_origin,
                                               forward, ref_alt, vertical_ref,
                                               rc);
        topo_sprite_cache.used_texture_path = true;
      } else {
        std::vector<ForwardViewTopography::Vertex> unused_fills;
        std::vector<ForwardViewTopography::Sprite> unused_sprites;
        ForwardViewTopography::BuildOverlay(topo_sprite_cache.fallback_lines,
                                            unused_fills, unused_sprites,
                                            *topography, map, build_origin,
                                            forward, ref_alt, vertical_ref, rc);
        topo_sprite_cache.used_texture_path = false;
      }
    }

    topo_sprite_cache.valid = true;
    topo_sprite_cache.anchor = build_origin;
    topo_sprite_cache.track = track;
    topo_sprite_cache.range = range;
    topo_sprite_cache.aspect = aspect;
    topo_sprite_cache.store_serial = store_serial;
    topo_sprite_cache.ref_alt = ref_alt;
  }

  sprites = topo_sprite_cache.sprites;

  double scroll_x = 0., scroll_y = 0.;
  if (topo_sprite_cache.anchor.IsValid() && start.IsValid()) {
    const GeoVector rel(topo_sprite_cache.anchor, start);
    const Angle rel_bearing = rel.bearing - track;
    scroll_x = rel.distance * rel_bearing.cos();
    scroll_y = rel.distance * rel_bearing.sin();
  }

  for (auto &sprite : sprites) {
    sprite.x -= float(scroll_x);
    sprite.y -= float(scroll_y);
    sprite.z += float(topo_sprite_cache.ref_alt - ref_alt);
  }

  if (!topo_sprite_cache.water_fills.empty()) {
    const float ref_alt_delta = float(topo_sprite_cache.ref_alt - ref_alt);

    std::vector<Vertex3D> fill_vertices;
    fill_vertices.reserve(topo_sprite_cache.water_fills.size());
    for (const auto &v : topo_sprite_cache.water_fills) {
      const double x = v.x - float(scroll_x);
      const double y = v.y - float(scroll_y);
      fill_vertices.push_back({
        float(x),
        GLfloat(ForwardViewGeometry::DisplayLateral(y)),
        v.z + ref_alt_delta,
        v.r, v.g, v.b, v.a,
      });
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    DrawVertices(fill_vertices, GL_TRIANGLES);
    glDisable(GL_BLEND);
  }

  if (topo_texture.IsValid() || topo_sprite_cache.fallback_lines.empty())
    return;

  const float ref_alt_delta = float(topo_sprite_cache.ref_alt - ref_alt);

  std::vector<Vertex3D> line_vertices;
  line_vertices.reserve(topo_sprite_cache.fallback_lines.size());
  for (const auto &v : topo_sprite_cache.fallback_lines) {
    const double x = v.x - float(scroll_x);
    const double y = v.y - float(scroll_y);
    Color color(unsigned(v.r * 255.f), unsigned(v.g * 255.f),
                unsigned(v.b * 255.f), unsigned(v.a * 255.f));
    const double z = v.z + ref_alt_delta;
    line_vertices.push_back({
      float(x),
      GLfloat(ForwardViewGeometry::DisplayLateral(y)),
      float(z),
      float(color.Red()) / 255.f,
      float(color.Green()) / 255.f,
      float(color.Blue()) / 255.f,
      float(color.Alpha()) / 255.f,
    });
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_TRUE);
  glLineWidth(float(Layout::ScaleFinePenWidth(1)));
  DrawVertices(line_vertices, GL_LINES);
  glLineWidth(1.f);
  glDisable(GL_BLEND);
}

void
ForwardViewRenderer::DrawAirspaces(const PixelRect &rc, double ref_alt,
                                   float aspect) noexcept
{
  if (airspaces == nullptr || airspaces->IsEmpty() || !forward.IsValid())
    return;

  const double vertical_ref = terrain_mesh.vertical_ref_alt;
  const double range = forward.distance;
  const Angle track = forward.bearing;
  const GeoPoint build_origin = terrain_mesh.anchor.IsValid()
    ? terrain_mesh.anchor
    : start;

  const AircraftState aircraft = ToAircraftState(Basic(), Calculated());
  const auto &computer_settings =
    CommonInterface::GetComputerSettings().airspace;

  std::vector<ForwardViewTopography::Vertex> fill_vertices;
  std::vector<ForwardViewTopography::Vertex> outline_vertices;
  const SunLight sun = GetForwardViewSunLight(Calculated(), Basic(),
                                              utc_offset, forward.bearing);
  ForwardViewAirspace::BuildVolumes(fill_vertices, outline_vertices,
                                    airspace_volume_cache,
                                    *airspaces, airspace_look,
                                    airspace_settings, computer_settings,
                                    aircraft,
                                    build_origin, track, range, aspect,
                                    ref_alt, vertical_ref,
                                    sun.dir_x, sun.dir_y, sun.dir_z,
                                    sun.active,
                                    rc);
  if (fill_vertices.empty() && outline_vertices.empty())
    return;

  double scroll_x = 0., scroll_y = 0.;
  if (build_origin.IsValid() && start.IsValid()) {
    const GeoVector rel(build_origin, start);
    const Angle rel_bearing = rel.bearing - track;
    scroll_x = rel.distance * rel_bearing.cos();
    scroll_y = rel.distance * rel_bearing.sin();
  }

  const float ref_alt_delta = float(airspace_volume_cache.ref_alt - ref_alt);

  auto convert = [&](const ForwardViewTopography::Vertex &v) {
    const double x = v.x - float(scroll_x);
    const double y = v.y - float(scroll_y);
    return Vertex3D{
      float(x),
      GLfloat(ForwardViewGeometry::DisplayLateral(y)),
      v.z + ref_alt_delta,
      v.r, v.g, v.b, v.a,
    };
  };

  if (!fill_vertices.empty()) {
    std::vector<Vertex3D> fills;
    fills.reserve(fill_vertices.size());
    for (const auto &v : fill_vertices)
      fills.push_back(convert(v));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    DrawVertices(fills, GL_TRIANGLES);
  }

  if (!outline_vertices.empty()) {
    std::vector<Vertex3D> outlines;
    outlines.reserve(outline_vertices.size());
    for (const auto &v : outline_vertices)
      outlines.push_back(convert(v));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glLineWidth(float(Layout::ScaleFinePenWidth(2)));
    DrawVertices(outlines, GL_LINES);
    glLineWidth(1.f);
    glDisable(GL_BLEND);
  } else if (!fill_vertices.empty())
    glDisable(GL_BLEND);
}

#ifdef HAVE_HTTP
void
ForwardViewRenderer::DrawXCThermVolumes(const PixelRect &rc, double ref_alt,
                                        float aspect) noexcept
{
  if (terrain == nullptr || !forward.IsValid())
    return;

  const double vertical_ref = terrain_mesh.vertical_ref_alt;
  const double range = forward.distance;
  const Angle track = forward.bearing;
  const GeoPoint build_origin = terrain_mesh.anchor.IsValid()
    ? terrain_mesh.anchor
    : start;

  RasterTerrain::Lease map_lease(*terrain);
  const RasterMap &map = map_lease;

  std::vector<ForwardViewTopography::Vertex> volume_vertices;
  ForwardViewXCTherm::BuildVolumes(volume_vertices, xctherm_volume_cache,
                                     build_origin, track, range, aspect,
                                     ref_alt, vertical_ref, &map, rc);
  if (volume_vertices.empty())
    return;

  double scroll_x = 0., scroll_y = 0.;
  if (build_origin.IsValid() && start.IsValid()) {
    const GeoVector rel(build_origin, start);
    const Angle rel_bearing = rel.bearing - track;
    scroll_x = rel.distance * rel_bearing.cos();
    scroll_y = rel.distance * rel_bearing.sin();
  }

  const float ref_alt_delta = float(xctherm_volume_cache.ref_alt - ref_alt);

  std::vector<Vertex3D> vertices;
  vertices.reserve(volume_vertices.size());
  for (const auto &v : volume_vertices) {
    const double x = v.x - float(scroll_x);
    const double y = v.y - float(scroll_y);
    vertices.push_back({
      float(x),
      GLfloat(ForwardViewGeometry::DisplayLateral(y)),
      v.z + ref_alt_delta,
      v.r, v.g, v.b, v.a,
    });
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_TRUE);
  DrawVertices(vertices, GL_TRIANGLES);
  glDisable(GL_BLEND);
}
#endif

#endif

void
ForwardViewRenderer::Paint(Canvas &canvas, const PixelRect &rc)
{
#ifndef ENABLE_OPENGL
  DrawVerticalGradient(canvas, rc,
                       look.sky_color, look.background_color,
                       look.background_color);
  canvas.SetTextColor(inverse ? COLOR_WHITE : look.text_color);
  const char *msg = _("Forward view requires OpenGL");
  const PixelSize size = canvas.CalcTextSize(msg);
  canvas.DrawText(rc.GetCenter() - size / 2u, msg);
  return;
#else
  if (!forward.IsValid() || !start.IsValid()) {
    canvas.Clear(SkyZenithColor(look.sky_color));
    canvas.SetTextColor(inverse ? COLOR_WHITE : look.text_color);
    const char *msg = _("No GPS");
    const PixelSize size = canvas.CalcTextSize(msg);
    canvas.DrawText(rc.GetCenter() - size / 2u, msg);
    return;
  }

  if (!gps_info.NavAltitudeAvailable()) {
    canvas.Clear(SkyZenithColor(look.sky_color));
    canvas.SetTextColor(inverse ? COLOR_WHITE : look.text_color);
    const char *msg = _("No altitude");
    const PixelSize size = canvas.CalcTextSize(msg);
    canvas.DrawText(rc.GetCenter() - size / 2u, msg);
    return;
  }

  PaintOpenGL(canvas, rc, gps_info.nav_altitude);
#endif
}
