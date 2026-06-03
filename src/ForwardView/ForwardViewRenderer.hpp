// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BaseBlackboard.hpp"
#include "ForwardView/ForwardViewGeometry.hpp"
#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Geo/GeoPoint.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Terrain/TerrainSettings.hpp"
#include "Terrain/Height.hpp"
#include "Math/Angle.hpp"
#include "util/Serial.hpp"
#include "time/PeriodClock.hpp"
#include "time/RoughTime.hpp"

#ifdef ENABLE_OPENGL
#include "ForwardView/ForwardViewTerrainShader.hpp"
#include "ForwardView/ForwardViewTopographyTexture.hpp"
#include "ForwardView/ForwardViewAirspaceOverlay.hpp"
#include "Topography/ForwardViewTopographyOverlay.hpp"
#ifdef HAVE_HTTP
#include "ForwardView/ForwardViewXCThermOverlay.hpp"
#endif
#include "ui/opengl/System.hpp"
#include "ui/canvas/Icon.hpp"
#include "ResourceId.hpp"

#include <glm/glm.hpp>
#endif

#include <vector>

struct PixelRect;
struct CrossSectionLook;
struct AirspaceLook;
struct MapSettings;
struct Color;
class Canvas;
class Airspaces;
class RasterTerrain;
class TopographyStore;
struct TopographyLook;

namespace ForwardViewTopography {
struct Sprite;
}

/**
 * EFIS-style forward-looking view: terrain mesh, glide path and task
 * guidance in a perspective 3D scene.
 */
class ForwardViewRenderer : public BaseBlackboard {
public:
#ifdef ENABLE_OPENGL
  struct Vertex3D {
    GLfloat x, y, z;
    GLfloat r, g, b, a;
  };
#endif

private:

  const CrossSectionLook &look;
  [[maybe_unused]] const AirspaceLook &airspace_look;
  [[maybe_unused]] const TopographyLook &topography_look;
  const bool inverse;

  GlideSettings glide_settings;
  GlidePolar glide_polar;

  TerrainRendererSettings terrain_settings;
  AirspaceRendererSettings airspace_settings;
  bool topography_enabled = false;
  bool airspace_enabled = false;
  RoughTimeDelta utc_offset{};

  const RasterTerrain *terrain = nullptr;
  TopographyStore *topography = nullptr;
  const Airspaces *airspaces = nullptr;

  GeoPoint start = GeoPoint::Invalid();
  GeoVector forward{ForwardViewGeometry::VIEW_RANGE_DEFAULT, Angle::Zero()};
  Angle smoothed_track = Angle::Zero();
  bool smoothed_track_valid = false;
  bool wireframe = false;

  GeoPoint motion_fix = GeoPoint::Invalid();
  Angle motion_track = Angle::Zero();
  double motion_speed = 0.;
  double view_range = ForwardViewGeometry::VIEW_RANGE_DEFAULT;
  PeriodClock motion_fix_clock;
  bool motion_valid = false;
  GeoPoint last_repaint_start = GeoPoint::Invalid();

#ifdef ENABLE_OPENGL
  struct TopoSpriteCache {
    bool valid = false;
    GeoPoint anchor = GeoPoint::Invalid();
    Angle track = Angle::Zero();
    double range = 0.;
    float aspect = 0.f;
    unsigned store_serial = 0;
    double ref_alt = 0.;
    std::vector<ForwardViewTopography::Sprite> sprites;
    std::vector<ForwardViewTopography::Vertex> fallback_lines;
    std::vector<ForwardViewTopography::Vertex> water_fills;
    bool used_texture_path = false;
  };

  mutable TopoSpriteCache topo_sprite_cache;
  mutable ForwardViewTopographyTexture::Cache topo_texture;
#ifdef HAVE_HTTP
  mutable ForwardViewXCTherm::VolumeCache xctherm_volume_cache;
#endif
  mutable ForwardViewAirspace::VolumeCache airspace_volume_cache;
#endif

#ifdef ENABLE_OPENGL
  struct TopoIconCacheEntry {
    ResourceId icon, big_icon, ultra_icon;
    MaskedIcon masked;
  };

  mutable std::vector<TopoIconCacheEntry> topo_icon_cache;

  struct TerrainMeshCache {
    bool valid = false;
    GeoPoint anchor = GeoPoint::Invalid();
    Angle track = Angle::Zero();
    double range = 0.;
    float aspect = 0.f;
    Serial terrain_serial;
    double dem_cell_spacing = 0.;
    /** Aircraft altitude when the mesh was built; vertical exaggeration datum. */
    double vertical_ref_alt = 0.;
    unsigned dist_count = 0;
    unsigned lateral_count = 0;
    std::vector<double> xs;
    std::vector<std::vector<double>> ys;
    std::vector<std::vector<double>> raw_heights;
    std::vector<std::vector<TerrainHeight>> terrain_heights;
    unsigned gpu_triangle_count = 0;
    unsigned gpu_ramp = 0;
    unsigned ramp = 0;
  };

  mutable TerrainMeshCache terrain_mesh;

  void BuildTerrainGpuMesh(glm::vec3 sun_dir, bool sun_active,
                           bool cast_shadows) const noexcept;

  void DrawTerrainGpu(double ref_alt, float eye_z,
                      glm::vec3 sun_dir, bool sun_active,
                      const glm::mat4 &projection,
                      const glm::mat4 &view) const noexcept;

  void InvalidateTerrainMesh() noexcept {
    terrain_mesh.valid = false;
    terrain_mesh.gpu_triangle_count = 0;
  }

  void EnsureTerrainMeshCache(double ref_alt, float aspect,
                              bool *rebuilt=nullptr) const noexcept;

  void DrawTerrainMesh(std::vector<Vertex3D> &opaque_vertices,
                       std::vector<Vertex3D> &fade_vertices,
                       double ref_alt, double eye_z,
                       const struct SunLight &sun) const noexcept;

  void DrawTerrainGrid(std::vector<Vertex3D> &vertices,
                       double ref_alt, double eye_z) const noexcept;

  void PaintOpenGL(Canvas &canvas, const PixelRect &rc,
                   double ref_alt) noexcept;

  void DrawTopography(const PixelRect &rc, double ref_alt,
                      std::vector<ForwardViewTopography::Sprite> &sprites) noexcept;

  const MaskedIcon &GetTopoIcon(ResourceId icon, ResourceId big_icon,
                                ResourceId ultra_icon) const noexcept;

  void DrawTopographySprites(const PixelRect &rc, double ref_alt,
                             const std::vector<ForwardViewTopography::Sprite> &sprites,
                             const glm::mat4 &projection,
                             const glm::mat4 &view) const noexcept;

#ifdef HAVE_HTTP
  void DrawXCThermVolumes(const PixelRect &rc, double ref_alt,
                          float aspect) noexcept;
#endif

  void DrawAirspaces(const PixelRect &rc, double ref_alt,
                     float aspect) noexcept;
#endif

public:
  ForwardViewRenderer(const CrossSectionLook &_look,
                      const AirspaceLook &_airspace_look,
                      const TopographyLook &_topography_look,
                      bool _inverse) noexcept;

  void ReadBlackboard(const MoreData &basic,
                      const DerivedInfo &calculated,
                      const GlideSettings &_glide_settings,
                      const GlidePolar &_glide_polar,
                      const MapSettings &map_settings,
                      RoughTimeDelta _utc_offset) noexcept;

  void SetTerrain(const RasterTerrain *_terrain) noexcept {
    terrain = _terrain;
  }

  void SetTopography(TopographyStore *_topography) noexcept {
    topography = _topography;
#ifdef ENABLE_OPENGL
    topo_texture.SetStore(_topography);
#endif
  }

  void SetAirspaces(const Airspaces *_airspaces) noexcept {
    airspaces = _airspaces;
  }

  void SetView(GeoPoint _start, Angle track, double range) noexcept;

  void SetMotionTarget(GeoPoint fix, Angle track, double ground_speed,
                       double range) noexcept;

  void TickMotion() noexcept;

  /** Advance motion; return true when the view moved enough to repaint. */
  bool TickMotionForRepaint() noexcept;

  void SetWireframe(bool _wireframe) noexcept {
    wireframe = _wireframe;
  }

  void SetInvalid() noexcept {
    forward.SetInvalid();
    start.SetInvalid();
    smoothed_track_valid = false;
    motion_valid = false;
    motion_fix.SetInvalid();
    motion_fix_clock.Reset();
    last_repaint_start.SetInvalid();
#ifdef ENABLE_OPENGL
    topo_sprite_cache.valid = false;
    topo_texture.Invalidate();
#ifdef HAVE_HTTP
    xctherm_volume_cache.valid = false;
#endif
    airspace_volume_cache.valid = false;
#endif
    InvalidateTerrainMesh();
  }

  void Paint(Canvas &canvas, const PixelRect &rc);
};
