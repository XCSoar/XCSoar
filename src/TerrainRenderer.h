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

#ifndef TERRAIN_RENDERER_H
#define TERRAIN_RENDERER_H

#include "Navigation/GeoPoint.hpp"

#include <windef.h>

class CSTScreenBuffer;
class Canvas;
class BGRColor;
class RasterMap;
class MapWindowProjection;
class RasterTerrain;
class RasterWeather;
class RasterRounding;
struct COLORRAMP;

class TerrainRenderer {
public:
  TerrainRenderer(const RasterTerrain *_terrain, RasterWeather *_weather,
                  RECT rc);
  ~TerrainRenderer();

public:
  POINT spot_max_pt;
  POINT spot_min_pt;
  short spot_max_val;
  short spot_min_val;

private:
  const RasterTerrain *terrain;
  RasterWeather *weather;

  unsigned int ixs, iys; // screen dimensions in coarse pixels
  unsigned int dtquant;
  unsigned int epx; // step size used for slope calculations

  RECT rect_visible;

  CSTScreenBuffer *sbuf;

  double pixelsize_d;

  int oversampling;
  int blursize;

  unsigned short *hBuf;
  BGRColor *colorBuf;
  bool do_shading;
  bool do_water;
  RasterMap *DisplayMap;
  RasterRounding *rounding;

  bool is_terrain;
  int interp_levels;
  const COLORRAMP *color_ramp;
  unsigned int height_scale;

  short TerrainRamp;
  short TerrainContrast;
  short TerrainBrightness;

private:
  void Height(MapWindowProjection &map_projection, bool isBigZoom);
  void ScanSpotHeights(const int X0, const int Y0, const int X1, const int Y1);
  void FillHeightBuffer(MapWindowProjection &map_projection,
			const int X0, const int Y0, const int X1, const int Y1);
  void Slope(const int sx, const int sy, const int sz);
  void ColorTable();
  void Draw(Canvas &canvas, RECT rc);

  /**
   * @param day_time the UTC time, in seconds since midnight
   */
  bool SetMap(const GEOPOINT &loc, int day_time);

 public:
  void SetSettings(short _TerrainRamp,
		   short _TerrainContrast,
		   short _TerrainBrightness) {
    TerrainRamp = _TerrainRamp;
    TerrainContrast= _TerrainContrast;
    TerrainBrightness = _TerrainBrightness;
  }

 public:
  /**
   * @param day_time the UTC time, in seconds since midnight
   */
  bool Draw(Canvas &canvas,
	    MapWindowProjection &map_projection,
	    const double sunazimuth, const double sunelevation,
            const GEOPOINT &loc, int day_time,
	    const bool isBigZoom);
};

#endif
