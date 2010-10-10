/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Projection.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/LabelBlock.hpp"
#include "Util/StringUtil.hpp"

BackgroundDrawHelper::BackgroundDrawHelper():
  m_rend(NULL),
  m_terrain(NULL),
  m_weather(NULL),
  m_sun_elevation(Angle::degrees(fixed(40))),
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
                           const Projection& proj,
                           const SETTINGS_MAP& settings_map)
{
  if (m_terrain == NULL) {
    // terrain may have been re-set, so may need new renderer
    reset();
    canvas.clear(Graphics::hBackgroundBrush);
    return;
  }
  if (!settings_map.EnableTerrain) {
    canvas.clear(Graphics::hBackgroundBrush);
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

  m_rend->SetSettings(settings_map.TerrainRamp,
                      settings_map.TerrainContrast,
                      settings_map.TerrainBrightness);
  m_rend->Draw(canvas, proj,
               m_sun_azimuth,
               m_sun_elevation);
}

void
BackgroundDrawHelper::sun_from_wind(const Projection& projection,
                                    const SpeedVector& wind)
{
  m_sun_elevation = Angle::degrees(fixed(40.0));
  // draw sun from constant angle if very low wind speed
  if (wind.norm < fixed_half) {
    m_sun_azimuth = projection.GetDisplayAngle() + Angle::degrees(fixed(45.0));
  } else {
    m_sun_azimuth = projection.GetDisplayAngle() - wind.bearing;
  }
}

void
BackgroundDrawHelper::DrawSpotHeight(Canvas &canvas,
                                     const Projection &map_projection,
                                     LabelBlock &block,
                                     const TCHAR *Buffer, POINT pt)
{
  if (string_is_empty(Buffer))
    return;

  POINT orig = map_projection.GetOrigScreen();
  RECT brect;
  SIZE tsize = canvas.text_size(Buffer);

  pt.x += 2 + orig.x;
  pt.y += 2 + orig.y;
  brect.left = pt.x;
  brect.right = brect.left + tsize.cx;
  brect.top = pt.y;
  brect.bottom = brect.top + tsize.cy;

  if (!block.check(brect))
    return;

  canvas.text(pt.x, pt.y, Buffer);
}

void
BackgroundDrawHelper::DrawSpotHeights(Canvas &canvas, 
                                      const Projection &proj,
                                      LabelBlock& block)
{
  if (m_weather == NULL || m_weather->GetParameter() == 0 ||
      m_rend == NULL)
    return;

  TCHAR Buffer[20];
  m_weather->ValueToText(Buffer, m_rend->spot_max_val);
  DrawSpotHeight(canvas, proj, block, Buffer, m_rend->spot_max_pt);

  m_weather->ValueToText(Buffer, m_rend->spot_min_val);
  DrawSpotHeight(canvas, proj, block, Buffer, m_rend->spot_min_pt);
}
