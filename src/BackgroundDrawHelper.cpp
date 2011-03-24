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
  m_rend(NULL),
  m_terrain(NULL),
  m_weather(NULL),
  m_sun_azimuth(Angle::degrees(fixed(45)))
{
}

BackgroundDrawHelper::~BackgroundDrawHelper()
{
  reset();
}

void
BackgroundDrawHelper::reset()
{
  delete m_rend;
  m_rend = NULL;
}


void 
BackgroundDrawHelper::set_terrain(const RasterTerrain *terrain)
{
  m_terrain = terrain;
  reset();
}

void 
BackgroundDrawHelper::set_weather(const RasterWeather *weather)
{
  m_weather = weather;
  reset();
}

void 
BackgroundDrawHelper::Draw(Canvas& canvas,
                           const WindowProjection& proj,
                           const SETTINGS_MAP& settings_map)
{
  if (m_terrain == NULL) {
    // terrain may have been re-set, so may need new renderer
    reset();
    canvas.clear_white();
    return;
  }
  if (!settings_map.EnableTerrain) {
    canvas.clear_white();
    return;
  }

  if (!m_rend) {
    // defer creation until first draw because
    // the buffer size, smoothing etc is set by the
    // loaded terrain properties
    if (m_weather) {
      m_rend = new WeatherTerrainRenderer(m_terrain, m_weather);
    } else {
      m_rend = new TerrainRenderer(m_terrain);
    }
  }

  m_rend->SetSettings(settings_map.SlopeShadingType != sstOff,
                      settings_map.TerrainRamp,
                      settings_map.TerrainContrast,
                      settings_map.TerrainBrightness);
  m_rend->Draw(canvas, proj,
               m_sun_azimuth);
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
  m_sun_azimuth = projection.GetScreenAngle() + angle;
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
  if (m_weather == NULL || m_weather->GetParameter() == 0 ||
      m_rend == NULL)
    return false;

  canvas.select(Fonts::Title);
  canvas.set_text_color(Color::BLACK);
  canvas.background_transparent();

  TCHAR Buffer[20];
  m_weather->ValueToText(Buffer, m_rend->spot_max_val);
  DrawSpotHeight(canvas, block, Buffer, m_rend->spot_max_pt);

  m_weather->ValueToText(Buffer, m_rend->spot_min_val);
  DrawSpotHeight(canvas, block, Buffer, m_rend->spot_min_pt);
  return true;
}
