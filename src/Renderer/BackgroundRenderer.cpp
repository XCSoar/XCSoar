// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackgroundRenderer.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "NMEA/Derived.hpp"

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
BackgroundRenderer::PrepareTerrainDraw(
  const WindowProjection &projection,
  const TerrainRendererSettings &settings,
  const DerivedInfo &calculated,
  bool high_resolution_preview) noexcept
{
  if (high_resolution_preview)
    SetFullResolution();
  SetShadingAngle(projection, settings, calculated);
}

void
BackgroundRenderer::PaintTerrainLayer(
  Canvas &canvas, const WindowProjection &projection,
  const TerrainRendererSettings &settings, const DerivedInfo &calculated,
  bool high_resolution_preview) noexcept
{
  PrepareTerrainDraw(projection, settings, calculated, high_resolution_preview);
  Draw(canvas, projection, settings);
}

void
BackgroundRenderer::Draw(Canvas& canvas,
                         const WindowProjection& proj,
                         const TerrainRendererSettings &terrain_settings) noexcept
{
  canvas.ClearWhite();

  if (terrain_settings.enable && terrain != nullptr) {
    if (!renderer) {
      // defer creation until first draw because
      // the buffer size, smoothing etc is set by the
      // loaded terrain properties
      renderer.reset(new TerrainRenderer(*terrain));

      if (full_resolution)
        renderer->SetQuantisationPixels(1);
    }

    renderer->SetSettings(terrain_settings);
    if (renderer->Generate(proj, shading_angle))
      renderer->Draw(canvas, proj);
  }
}

void
BackgroundRenderer::SetShadingAngle(const WindowProjection& projection,
                                    const TerrainRendererSettings &settings,
                                    const DerivedInfo &calculated) noexcept
{
  Angle angle;

  if (settings.slope_shading == SlopeShading::TOP_LEFT)
    angle = DEFAULT_SHADING_ANGLE + projection.GetScreenAngle();

  else if (settings.slope_shading == SlopeShading::WIND &&
      calculated.wind_available &&
      calculated.wind.norm >= 0.5)
    angle = calculated.wind.bearing;

  else if (settings.slope_shading == SlopeShading::SUN &&
           calculated.sun_data_available)
    angle = calculated.sun_azimuth;

  else
    angle = DEFAULT_SHADING_ANGLE;

  SetShadingAngle(projection, angle);
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
