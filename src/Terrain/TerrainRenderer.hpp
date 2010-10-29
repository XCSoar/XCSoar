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

#ifndef XCSOAR_TERRAIN_RENDERER_HPP
#define XCSOAR_TERRAIN_RENDERER_HPP

#include "RasterRenderer.hpp"
#include "Util/NonCopyable.hpp"

#include <windef.h>

class Canvas;
class WindowProjection;
class RasterTerrain;
struct COLORRAMP;

class TerrainRenderer : private NonCopyable {
protected:
  RasterRenderer raster_renderer;

public:
  TerrainRenderer(const RasterTerrain *_terrain);

public:
  POINT spot_max_pt;
  POINT spot_min_pt;
  short spot_max_val;
  short spot_min_val;

protected:
  const COLORRAMP *last_color_ramp;

private:
  const RasterTerrain *terrain;

protected:
  short TerrainRamp;
  short TerrainContrast;
  short TerrainBrightness;

  void ScanSpotHeights(const RECT& rect);

  /**
   * Convert the height matrix into the image, without shading.
   */
  void GenerateImage();

  /**
   * Convert the height matrix into the image, with slope shading.
   */
  void GenerateSlopeImage(const int sx, const int sy, const int sz);

  /**
   * Convert the height matrix into the image, with slope shading.
   */
  void GenerateSlopeImage(const Angle sunazimuth);

  void ColorTable();
  void CopyTo(Canvas &canvas, unsigned width, unsigned height);

  virtual bool do_scan_spot();

public:
  void
  SetSettings(short _TerrainRamp, short _TerrainContrast,
      short _TerrainBrightness)
  {
    TerrainRamp = _TerrainRamp;
    TerrainContrast = _TerrainContrast;
    TerrainBrightness = _TerrainBrightness;
  }

  virtual void Draw(Canvas &canvas, const WindowProjection &map_projection,
                    const Angle sunazimuth);
};

#endif
