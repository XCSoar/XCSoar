/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "BackgroundRenderer.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Canvas.hpp"
#include "NMEA/Derived.hpp"

const Angle BackgroundRenderer::DEFAULT_SHADING_ANGLE = Angle::Degrees(-45);

BackgroundRenderer::BackgroundRenderer() {}
BackgroundRenderer::~BackgroundRenderer() {}

void
BackgroundRenderer::Flush()
{
  if (renderer != nullptr)
    renderer->Flush();
}

void
BackgroundRenderer::SetTerrain(const RasterTerrain *_terrain)
{
  terrain = _terrain;
  renderer.reset();
}

void
BackgroundRenderer::Draw(Canvas& canvas,
                         const WindowProjection& proj,
                         const TerrainRendererSettings &terrain_settings)
{
  canvas.ClearWhite();

  if (terrain_settings.enable && terrain != nullptr) {
    if (!renderer)
      // defer creation until first draw because
      // the buffer size, smoothing etc is set by the
      // loaded terrain properties
      renderer.reset(new TerrainRenderer(*terrain));

    renderer->SetSettings(terrain_settings);
    if (renderer->Generate(proj, shading_angle))
      renderer->Draw(canvas, proj);
  }
}

void
BackgroundRenderer::SetShadingAngle(const WindowProjection& projection,
                                    const TerrainRendererSettings &settings,
                                    const DerivedInfo &calculated)
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
}

void
BackgroundRenderer::SetShadingAngle(const WindowProjection& projection,
                                    Angle angle)
{
#ifdef ENABLE_OPENGL
  /* on OpenGL, the texture is rotated to apply the screen angle */
  shading_angle = angle;
#else
  shading_angle = angle - projection.GetScreenAngle();
#endif
}
