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
    if (generated)
      ++perf_generate_true;

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
      LogFmt("Terrain perf: frames={} generate avg={:.3f}ms max={:.3f}ms, "
             "draw avg={:.3f}ms max={:.3f}ms, draw_calls={}, "
             "q_pixels={} q_effective={} shader={}",
             perf_frames,
             (double)perf_generate_us_sum / frames / 1000.0,
             (double)perf_generate_us_max / 1000.0,
             (double)perf_draw_us_sum / draws / 1000.0,
             (double)perf_draw_us_max / 1000.0,
             perf_generate_true,
             renderer->GetQuantisationPixels(),
             renderer->GetQuantisationEffective(),
#ifdef ENABLE_OPENGL
             renderer->IsShaderShadingEnabled() ? "on" : "off"
#else
             "off"
#endif
      );

      perf_frames = 0;
      perf_generate_true = 0;
      perf_generate_us_sum = 0;
      perf_generate_us_max = 0;
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
