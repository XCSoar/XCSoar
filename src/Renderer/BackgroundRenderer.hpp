// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"

#include <memory>

class Canvas;
class WindowProjection;
struct TerrainRendererSettings;
class TerrainRenderer;
class RasterTerrain;
struct DerivedInfo;

/**
 * Utility class to draw terrain, topography (not implemented yet)
 */
class BackgroundRenderer {
  static const Angle DEFAULT_SHADING_ANGLE;

  const RasterTerrain *terrain = nullptr;
  std::unique_ptr<TerrainRenderer> renderer;
  Angle shading_angle = DEFAULT_SHADING_ANGLE;

public:
  BackgroundRenderer();

  ~BackgroundRenderer();

  /**
   * Flush all caches.
   */
  void Flush();

  void Draw(Canvas& canvas,
            const WindowProjection& proj,
            const TerrainRendererSettings &terrain_settings);

  void SetShadingAngle(const WindowProjection &projection,
                       const TerrainRendererSettings &settings,
                       const DerivedInfo &calculated);
  void SetTerrain(const RasterTerrain *terrain);

private:
  void SetShadingAngle(const WindowProjection& proj, Angle angle);
};
