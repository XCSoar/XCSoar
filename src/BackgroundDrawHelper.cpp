/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "BackgroundDrawHelper.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Terrain/WeatherTerrainRenderer.hpp"
#include "SettingsMap.hpp"
#include "Navigation/SpeedVector.hpp"
#include "WindowProjection.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/LabelBlock.hpp"
#include "Screen/Fonts.hpp"
#include "Util/StringUtil.hpp"

BackgroundDrawHelper::BackgroundDrawHelper():
  terrain(NULL),
  weather(NULL),
  renderer(NULL),
  sun_azimuth(Angle::degrees(fixed(45)))
{
}

BackgroundDrawHelper::~BackgroundDrawHelper()
{
  reset();
}

void
BackgroundDrawHelper::reset()
{
  delete renderer;
  renderer = NULL;
}


void 
BackgroundDrawHelper::set_terrain(const RasterTerrain *_terrain)
{
  terrain = _terrain;
  reset();
}

void 
BackgroundDrawHelper::set_weather(const RasterWeather *_weather)
{
  weather = _weather;
  reset();
}

void 
BackgroundDrawHelper::Draw(Canvas& canvas,
                           const WindowProjection& proj,
                           const TerrainRendererSettings &terrain_settings)
{
  if (terrain == NULL) {
    // terrain may have been re-set, so may need new renderer
    reset();
    canvas.clear_white();
    return;
  }
  if (!terrain_settings.enable) {
    canvas.clear_white();
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
  renderer->Draw(canvas, proj,
                 sun_azimuth);
}

void
BackgroundDrawHelper::sun_from_wind(const WindowProjection& projection,
                                    const SpeedVector& wind)
{
  // draw sun from constant angle if very low wind speed
  if (wind.norm < fixed_half)
    set_sun_angle(projection, Angle::degrees(fixed(-45.0)));
  else
    set_sun_angle(projection, wind.bearing);
}

void
BackgroundDrawHelper::set_sun_angle(const WindowProjection& projection,
                                    const Angle& angle)
{
  sun_azimuth = angle - projection.GetScreenAngle();
}

void
BackgroundDrawHelper::DrawSpotHeight(Canvas &canvas, LabelBlock &block,
                                     const TCHAR *Buffer, RasterPoint pt)
{
  if (string_is_empty(Buffer))
    return;

  PixelRect brect;
  PixelSize tsize = canvas.text_size(Buffer);

  pt.x += 2;
  pt.y += 2;
  brect.left = pt.x;
  brect.right = brect.left + tsize.cx;
  brect.top = pt.y;
  brect.bottom = brect.top + tsize.cy;

  if (!block.check(brect))
    return;

  canvas.text(pt.x, pt.y, Buffer);
}

bool
BackgroundDrawHelper::DrawSpotHeights(Canvas &canvas, 
                                      LabelBlock& block)
{
  if (weather == NULL || weather->GetParameter() == 0 ||
      renderer == NULL)
    return false;

  canvas.select(Fonts::Title);
  canvas.set_text_color(COLOR_BLACK);
  canvas.background_transparent();

  TCHAR Buffer[20];
  weather->ValueToText(Buffer, renderer->spot_max_val);
  DrawSpotHeight(canvas, block, Buffer, renderer->spot_max_pt);

  weather->ValueToText(Buffer, renderer->spot_min_val);
  DrawSpotHeight(canvas, block, Buffer, renderer->spot_min_pt);
  return true;
}
