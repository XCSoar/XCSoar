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
 * Utility class to draw terrain, topography.
 */
class BackgroundRenderer {
  static constexpr Angle DEFAULT_SHADING_ANGLE = Angle::Degrees(-45);

  const RasterTerrain *terrain = nullptr;
  std::unique_ptr<TerrainRenderer> renderer;
  Angle shading_angle = DEFAULT_SHADING_ANGLE;

public:
  BackgroundRenderer() noexcept;
  ~BackgroundRenderer() noexcept;

  /**
   * Flush all caches.
   */
  void Flush() noexcept;

  void Draw(Canvas& canvas,
            const WindowProjection& proj,
            const TerrainRendererSettings &terrain_settings) noexcept;

  void SetShadingAngle(const WindowProjection &projection,
                       const TerrainRendererSettings &settings,
                       const DerivedInfo &calculated) noexcept;
  void SetTerrain(const RasterTerrain *terrain) noexcept;

private:
  void SetShadingAngle(const WindowProjection& proj, Angle angle) noexcept;
};
