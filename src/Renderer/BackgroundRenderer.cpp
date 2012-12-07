/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Terrain/WeatherTerrainRenderer.hpp"
#include "MapSettings.hpp"
#include "Geo/SpeedVector.hpp"
#include "Projection/WindowProjection.hpp"
#include "Screen/Canvas.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Look/Fonts.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"

BackgroundRenderer::BackgroundRenderer():
  terrain(NULL),
  weather(NULL),
  renderer(NULL),
  shading_angle(Angle::Degrees(fixed(-45)))
{
}

BackgroundRenderer::~BackgroundRenderer()
{
  Reset();
}

void
BackgroundRenderer::Reset()
{
  delete renderer;
  renderer = NULL;
}


void 
BackgroundRenderer::SetTerrain(const RasterTerrain *_terrain)
{
  terrain = _terrain;
  Reset();
}

void 
BackgroundRenderer::SetWeather(const RasterWeather *_weather)
{
  weather = _weather;
  Reset();
}

void 
BackgroundRenderer::Draw(Canvas& canvas,
                         const WindowProjection& proj,
                         const TerrainRendererSettings &terrain_settings)
{
  if (terrain == NULL) {
    // terrain may have been re-set, so may need new renderer
    Reset();
    canvas.ClearWhite();
    return;
  }
  if (!terrain_settings.enable) {
    canvas.ClearWhite();
    return;
  }

  if (!renderer) {
    // defer creation until first draw because
    // the buffer size, smoothing etc is set by the
    // loaded terrain properties
    if (weather) {
      renderer = new WeatherTerrainRenderer(terrain, weather);
    } else {
      renderer = new TerrainRenderer(terrain);
    }
  }

  renderer->SetSettings(terrain_settings);
  renderer->Generate(proj, shading_angle);
  renderer->Draw(canvas, proj);
}

void
BackgroundRenderer::SetShadingAngle(const WindowProjection& projection,
                                    const TerrainRendererSettings &settings,
                                    const DerivedInfo &calculated)
{
  if (settings.slope_shading == SlopeShading::WIND &&
      calculated.wind_available &&
      calculated.wind.norm >= fixed(0.5))
    SetShadingAngle(projection, calculated.wind.bearing);

  else if (settings.slope_shading == SlopeShading::SUN &&
           calculated.sun_data_available)
    SetShadingAngle(projection, calculated.sun_azimuth);

  else
    SetShadingAngle(projection, Angle::Degrees(fixed(-45.0)));

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

void
BackgroundRenderer::DrawSpotHeight(Canvas &canvas, LabelBlock &block,
                                   const TCHAR *buffer, RasterPoint pt)
{
  if (StringIsEmpty(buffer))
    return;

  PixelRect block_rect;
  PixelSize text_size = canvas.CalcTextSize(buffer);

  pt.x += 2;
  pt.y += 2;
  block_rect.left = pt.x;
  block_rect.right = block_rect.left + text_size.cx;
  block_rect.top = pt.y;
  block_rect.bottom = block_rect.top + text_size.cy;

  if (!block.check(block_rect))
    return;

  canvas.DrawText(pt.x, pt.y, buffer);
}

bool
BackgroundRenderer::DrawSpotHeights(Canvas &canvas, LabelBlock& block)
{
  if (weather == NULL || weather->GetParameter() == 0 || renderer == NULL)
    return false;

  canvas.Select(Fonts::title);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();

  TCHAR buffer[20];
  weather->ValueToText(buffer, renderer->spot_max_val);
  DrawSpotHeight(canvas, block, buffer, renderer->spot_max_pt);

  weather->ValueToText(buffer, renderer->spot_min_val);
  DrawSpotHeight(canvas, block, buffer, renderer->spot_min_pt);
  return true;
}
