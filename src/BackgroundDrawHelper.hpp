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

#ifndef BACKGROUND_DRAW_HELPER_HPP
#define BACKGROUND_DRAW_HELPER_HPP

#include "Screen/Point.hpp"

class Canvas;
class WindowProjection;
struct SETTINGS_MAP;
class TerrainRenderer;
struct SpeedVector;
class RasterTerrain;
class RasterWeather;
class LabelBlock;

#include "Math/Angle.hpp"

#include <tchar.h>

/**
 * Utility class to draw terrain, topography (not implemented yet)
 */
class BackgroundDrawHelper 
{
  TerrainRenderer* m_rend;
  const RasterTerrain *m_terrain;
  const RasterWeather *m_weather;
  Angle m_sun_azimuth;

public:
  BackgroundDrawHelper();
  ~BackgroundDrawHelper();

  void Draw(Canvas& canvas,
            const WindowProjection& proj,
            const SETTINGS_MAP& settings_map);

  bool DrawSpotHeights(Canvas& canvas, LabelBlock& block);

  void sun_from_wind(const WindowProjection& proj,
                     const SpeedVector& wind);
  void set_sun_angle(const WindowProjection& proj,
                     const Angle &angle);
  void reset();
  void set_terrain(const RasterTerrain *terrain);
  void set_weather(const RasterWeather *weather);

private:
  static void DrawSpotHeight(Canvas &canvas,  LabelBlock &block,
                             const TCHAR *Buffer, RasterPoint pt);
};

#endif
