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

#ifndef BACKGROUND_DRAW_HELPER_HPP
#define BACKGROUND_DRAW_HELPER_HPP

class Canvas;
class WindowProjection;
struct SETTINGS_MAP;
class TerrainRenderer;
struct SpeedVector;
class RasterTerrain;
class RasterWeather;
class LabelBlock;

#include "Math/Angle.hpp"
#include <windef.h>

/**
 * Utility class to draw terrain, topology (not implemented yet)
 */
class BackgroundDrawHelper 
{
public:
  BackgroundDrawHelper();
  ~BackgroundDrawHelper();

  void Draw(Canvas& canvas,
            const WindowProjection& proj,
            const SETTINGS_MAP& settings_map);

  void DrawSpotHeights(Canvas& canvas,
                       const WindowProjection &proj,
                       LabelBlock& block);

  void sun_from_wind(const WindowProjection& proj,
                     const SpeedVector& wind);
  void reset();
  void set_terrain(const RasterTerrain *terrain);
  void set_weather(const RasterWeather *weather);

private:
  static void DrawSpotHeight(Canvas &canvas, const WindowProjection &map_projection,
                             LabelBlock &block, const TCHAR *Buffer, POINT pt);

private:
  TerrainRenderer* m_rend;
  const RasterTerrain *m_terrain;
  const RasterWeather *m_weather;
  Angle m_sun_elevation;
  Angle m_sun_azimuth;
};

#endif
