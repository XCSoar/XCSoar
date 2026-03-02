// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RasterRenderer.hpp"
#include "util/Serial.hpp"
#include "Terrain/TerrainSettings.hpp"

#include <cstdint>

#ifndef ENABLE_OPENGL
#include "Projection/CompareProjection.hpp"
#endif

class Canvas;
class WindowProjection;
class RasterTerrain;
struct ColorRamp;

class TerrainRenderer {
  const RasterTerrain &terrain;

  Serial terrain_serial;

protected:
  struct TerrainRendererSettings settings;

#ifndef ENABLE_OPENGL
  CompareProjection compare_projection;
#endif

  Angle last_sun_azimuth = Angle::Zero();

  const ColorRamp *last_color_ramp = nullptr;
  bool last_generate_cache_hit = false;
  std::uint32_t last_scan_map_us = 0;
  std::uint32_t last_generate_image_us = 0;

  RasterRenderer raster_renderer;

public:
  TerrainRenderer(const RasterTerrain &_terrain);
  ~TerrainRenderer() {}

  TerrainRenderer(const TerrainRenderer &) = delete;
  TerrainRenderer &operator=(const TerrainRenderer &) = delete;

  /**
   * Flush the cache.
   */
  void Flush() {
#ifdef ENABLE_OPENGL
    raster_renderer.Invalidate();
#else
    compare_projection.Clear();
#endif
  }

public:
  const TerrainRendererSettings &GetSettings() const {
    return settings;
  }

  void SetSettings(const TerrainRendererSettings &_settings) {
    settings = _settings;
  }

#ifdef ENABLE_OPENGL
  /**
   * Force a fixed quantisation value, bypassing the idle-based
   * dynamic adjustment.  Call with q=1 for preview windows that
   * should always render at full resolution.
   */
  void SetQuantisationPixels(unsigned q) noexcept {
    raster_renderer.SetQuantisationPixels(q);
  }
#endif

  /**
   * @return true if an image has been renderered and Draw() may be
   * called
   */
  bool Generate(const WindowProjection &map_projection,
                const Angle sunazimuth);

  void Draw(Canvas &canvas, const WindowProjection &projection) const {
    raster_renderer.Draw(canvas, projection);
  }

  unsigned GetQuantisationPixels() const noexcept {
    return raster_renderer.GetQuantisationPixels();
  }

  unsigned GetQuantisationEffective() const noexcept {
    return raster_renderer.GetQuantisationEffective();
  }

  bool WasLastGenerateCacheHit() const noexcept {
    return last_generate_cache_hit;
  }

  std::uint32_t GetLastScanMapUs() const noexcept {
    return last_scan_map_us;
  }

  std::uint32_t GetLastGenerateImageUs() const noexcept {
    return last_generate_image_us;
  }

#ifdef ENABLE_OPENGL
  bool IsShaderShadingEnabled() const noexcept {
    return raster_renderer.IsShaderShadingEnabled();
  }

  bool IsShaderRampEnabled() const noexcept {
    return raster_renderer.IsShaderRampEnabled();
  }
#endif
};
