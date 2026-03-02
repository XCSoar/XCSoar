// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"

#include <array>
#include <cstdint>
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

  unsigned perf_frames = 0;
  unsigned perf_generate_true = 0;
  unsigned perf_generate_cache_hits = 0;
  unsigned perf_generate_updates = 0;
  std::uint64_t perf_generate_us_sum = 0;
  std::uint64_t perf_generate_us_max = 0;
  std::uint64_t perf_draw_us_sum = 0;
  std::uint64_t perf_draw_us_max = 0;
  std::array<std::uint32_t, 120> perf_generate_samples_us{};
  std::array<std::uint32_t, 120> perf_scan_samples_us{};
  std::array<std::uint32_t, 120> perf_image_samples_us{};
  std::uint64_t perf_scan_us_sum = 0;
  std::uint64_t perf_scan_us_max = 0;
  std::uint64_t perf_image_us_sum = 0;
  std::uint64_t perf_image_us_max = 0;

#ifdef ENABLE_OPENGL
  /** force full terrain resolution regardless of user idle state */
  bool full_resolution = false;
#endif

public:
  BackgroundRenderer() noexcept;
  ~BackgroundRenderer() noexcept;

#ifdef ENABLE_OPENGL
  /**
   * Force full terrain resolution.  Useful for static renderings
   * (analysis dialog, previews) where the idle-based dynamic
   * quantisation would produce blocky images.
   */
  void SetFullResolution() noexcept {
    full_resolution = true;
  }
#endif

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
