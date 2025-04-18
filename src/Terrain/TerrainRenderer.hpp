// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RasterRenderer.hpp"
#include "util/Serial.hpp"
#include "Terrain/TerrainSettings.hpp"

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

  /**
   * @return true if an image has been renderered and Draw() may be
   * called
   */
  bool Generate(const WindowProjection &map_projection,
                const Angle sunazimuth);

  void Draw(Canvas &canvas, const WindowProjection &projection) const {
    raster_renderer.Draw(canvas, projection);
  }
};
