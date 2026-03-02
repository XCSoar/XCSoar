// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackgroundRenderer.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "NMEA/Derived.hpp"
#include "LogFile.hpp"

#include <algorithm>
#include <chrono>

/**
 * Temporary perf-testing toggle:
 * continuously rotate terrain shading to force frequent regenerations.
 */
static constexpr bool ENABLE_ROTATING_TERRAIN_SHADING = true;
static constexpr unsigned ROTATING_TERRAIN_HZ = 32;
static constexpr double ROTATING_TERRAIN_DEG_PER_TICK = 1.0;

[[gnu::pure]]
static std::uint64_t
PercentileFromSorted(const std::uint32_t *sorted,
                     unsigned count, unsigned p) noexcept
{
  if (count == 0)
    return 0;

  const unsigned index = (count - 1) * p / 100;
  return sorted[index];
}

BackgroundRenderer::BackgroundRenderer() noexcept = default;
BackgroundRenderer::~BackgroundRenderer() noexcept = default;

void
BackgroundRenderer::Flush() noexcept
{
  if (renderer != nullptr)
    renderer->Flush();
}

void
BackgroundRenderer::SetTerrain(const RasterTerrain *_terrain) noexcept
{
  terrain = _terrain;
  renderer.reset();
}

void
BackgroundRenderer::Draw(Canvas& canvas,
                         const WindowProjection& proj,
                         const TerrainRendererSettings &terrain_settings) noexcept
{
  using clock = std::chrono::steady_clock;

  canvas.ClearWhite();

  if (terrain_settings.enable && terrain != nullptr) {
    if (!renderer) {
      // defer creation until first draw because
      // the buffer size, smoothing etc is set by the
      // loaded terrain properties
      renderer.reset(new TerrainRenderer(*terrain));

#ifdef ENABLE_OPENGL
      if (full_resolution)
        renderer->SetQuantisationPixels(1);
#endif
    }

    renderer->SetSettings(terrain_settings);
    const auto t0 = clock::now();
    const bool generated = renderer->Generate(proj, shading_angle);
    const auto t1 = clock::now();

    const std::uint64_t generate_us =
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    perf_generate_us_sum += generate_us;
    perf_generate_us_max = std::max(perf_generate_us_max, generate_us);
    perf_generate_samples_us[perf_frames] = (std::uint32_t)
      std::min<std::uint64_t>(generate_us, UINT32_MAX);
    if (generated)
      ++perf_generate_true;
    if (generated && renderer->WasLastGenerateCacheHit()) {
      ++perf_generate_cache_hits;
    } else if (generated && perf_generate_updates < perf_scan_samples_us.size()) {
      const std::uint64_t scan_us = renderer->GetLastScanMapUs();
      const std::uint64_t image_us = renderer->GetLastGenerateImageUs();
      perf_scan_us_sum += scan_us;
      perf_scan_us_max = std::max(perf_scan_us_max, scan_us);
      perf_image_us_sum += image_us;
      perf_image_us_max = std::max(perf_image_us_max, image_us);
      perf_scan_samples_us[perf_generate_updates] = (std::uint32_t)
        std::min<std::uint64_t>(scan_us, UINT32_MAX);
      perf_image_samples_us[perf_generate_updates] = (std::uint32_t)
        std::min<std::uint64_t>(image_us, UINT32_MAX);
      ++perf_generate_updates;
    }

    std::uint64_t draw_us = 0;
    if (generated) {
      const auto t2 = clock::now();
      renderer->Draw(canvas, proj);
      const auto t3 = clock::now();
      draw_us = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2)
        .count();
      perf_draw_us_sum += draw_us;
      perf_draw_us_max = std::max(perf_draw_us_max, draw_us);
    }

    ++perf_frames;
    if (perf_frames >= 120) {
      const double frames = perf_frames > 0 ? perf_frames : 1;
      const double draws = perf_generate_true > 0 ? perf_generate_true : 1;
      auto sorted_generate = perf_generate_samples_us;
      std::sort(sorted_generate.begin(), sorted_generate.end());
      const double generate_p95_ms =
        (double)PercentileFromSorted(sorted_generate.data(),
                                     perf_frames, 95) / 1000.0;
      const double generate_p99_ms =
        (double)PercentileFromSorted(sorted_generate.data(),
                                     perf_frames, 99) / 1000.0;
      const double updates = perf_generate_updates > 0
        ? perf_generate_updates : 1;
      auto sorted_scan = perf_scan_samples_us;
      auto sorted_image = perf_image_samples_us;
      std::sort(sorted_scan.begin(),
                sorted_scan.begin() + perf_generate_updates);
      std::sort(sorted_image.begin(),
                sorted_image.begin() + perf_generate_updates);
      const double scan_p95_ms =
        (double)PercentileFromSorted(sorted_scan.data(),
                                     perf_generate_updates, 95) / 1000.0;
      const double scan_p99_ms =
        (double)PercentileFromSorted(sorted_scan.data(),
                                     perf_generate_updates, 99) / 1000.0;
      const double image_p95_ms =
        (double)PercentileFromSorted(sorted_image.data(),
                                     perf_generate_updates, 95) / 1000.0;
      const double image_p99_ms =
        (double)PercentileFromSorted(sorted_image.data(),
                                     perf_generate_updates, 99) / 1000.0;

      LogFmt("Terrain perf: frames={} generate avg={:.3f}ms "
             "p95={:.3f}ms p99={:.3f}ms max={:.3f}ms, "
             "regen={} cache_hits={}, "
             "scan avg={:.3f}ms p95={:.3f}ms p99={:.3f}ms max={:.3f}ms, "
             "image avg={:.3f}ms p95={:.3f}ms p99={:.3f}ms max={:.3f}ms, "
             "draw avg={:.3f}ms max={:.3f}ms, draw_calls={}, "
             "q_pixels={} q_effective={} shader={} ramp={}",
             perf_frames,
             (double)perf_generate_us_sum / frames / 1000.0,
             generate_p95_ms,
             generate_p99_ms,
             (double)perf_generate_us_max / 1000.0,
             perf_generate_updates,
             perf_generate_cache_hits,
             (double)perf_scan_us_sum / updates / 1000.0,
             scan_p95_ms,
             scan_p99_ms,
             (double)perf_scan_us_max / 1000.0,
             (double)perf_image_us_sum / updates / 1000.0,
             image_p95_ms,
             image_p99_ms,
             (double)perf_image_us_max / 1000.0,
             (double)perf_draw_us_sum / draws / 1000.0,
             (double)perf_draw_us_max / 1000.0,
             perf_generate_true,
             renderer->GetQuantisationPixels(),
             renderer->GetQuantisationEffective(),
#ifdef ENABLE_OPENGL
             renderer->IsShaderShadingEnabled() ? "on" : "off",
             renderer->IsShaderRampEnabled() ? "on" : "off"
#else
             "off", "off"
#endif
      );

      perf_frames = 0;
      perf_generate_true = 0;
      perf_generate_cache_hits = 0;
      perf_generate_updates = 0;
      perf_generate_us_sum = 0;
      perf_generate_us_max = 0;
      perf_scan_us_sum = 0;
      perf_scan_us_max = 0;
      perf_image_us_sum = 0;
      perf_image_us_max = 0;
      perf_draw_us_sum = 0;
      perf_draw_us_max = 0;
    }
  }
}

void
BackgroundRenderer::SetShadingAngle(const WindowProjection& projection,
                                    const TerrainRendererSettings &settings,
                                    const DerivedInfo &calculated) noexcept
{
  using clock = std::chrono::steady_clock;
  Angle angle;

  if (settings.slope_shading == SlopeShading::WIND &&
      calculated.wind_available &&
      calculated.wind.norm >= 0.5)
    angle = calculated.wind.bearing;

  else if (settings.slope_shading == SlopeShading::SUN &&
           calculated.sun_data_available)
    angle = calculated.sun_azimuth;

  else
    angle = DEFAULT_SHADING_ANGLE;

#ifdef ENABLE_OPENGL
  if (ENABLE_ROTATING_TERRAIN_SHADING &&
      settings.slope_shading != SlopeShading::OFF) {
    static const auto start = clock::now();
    const auto now = clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<
      std::chrono::milliseconds>(now - start).count();
    const auto ticks = (std::uint64_t)elapsed_ms * ROTATING_TERRAIN_HZ / 1000u;
    angle += Angle::Degrees((double)ticks * ROTATING_TERRAIN_DEG_PER_TICK);
  }
#endif

  SetShadingAngle(projection, angle);

#ifdef ENABLE_OPENGL
  if (settings.slope_shading == SlopeShading::FIXED) {
    /* Temporary OpenGL mockup: make "Fixed" behave as screen-fixed
       illumination (lamp attached to the display). */
    shading_angle = angle - projection.GetScreenAngle();
  }
#endif
}

inline void
BackgroundRenderer::SetShadingAngle([[maybe_unused]] const WindowProjection& projection,
                                    Angle angle) noexcept
{
#ifdef ENABLE_OPENGL
  /* on OpenGL, the texture is rotated to apply the screen angle */
  shading_angle = angle;
#else
  shading_angle = angle - projection.GetScreenAngle();
#endif
}
