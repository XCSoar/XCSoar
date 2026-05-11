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

  /** force full terrain resolution regardless of user idle state */
  bool full_resolution = false;

public:
  BackgroundRenderer() noexcept;
  ~BackgroundRenderer() noexcept;

  /**
   * Force full terrain resolution.  Useful for static renderings
   * (analysis dialog, previews) where the idle-based dynamic
   * quantisation would produce blocky images.
   */
  void SetFullResolution() noexcept {
    full_resolution = true;
  }

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

  /**
   * Configure slope shading (and optionally force full-resolution terrain
   * quantisation for static previews) before #Draw().
   * @param high_resolution_preview if true, calls #SetFullResolution()
   */
  void PrepareTerrainDraw(const WindowProjection &projection,
                          const TerrainRendererSettings &settings,
                          const DerivedInfo &calculated,
                          bool high_resolution_preview) noexcept;

  /**
   * Configure shading (#PrepareTerrainDraw) then paint terrain (#Draw).
   */
  void PaintTerrainLayer(Canvas &canvas,
                         const WindowProjection &projection,
                         const TerrainRendererSettings &settings,
                         const DerivedInfo &calculated,
                         bool high_resolution_preview) noexcept;

private:
  void SetShadingAngle(const WindowProjection& proj, Angle angle) noexcept;
};
