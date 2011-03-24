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

#ifndef XCSOAR_TERRAIN_RENDERER_HPP
#define XCSOAR_TERRAIN_RENDERER_HPP

#include "RasterRenderer.hpp"
#include "Util/NonCopyable.hpp"

class Canvas;
class WindowProjection;
class RasterTerrain;
struct ColorRamp;

class TerrainRenderer : private NonCopyable {
protected:
  RasterRenderer raster_renderer;

public:
  TerrainRenderer(const RasterTerrain *_terrain);

public:
  RasterPoint spot_max_pt;
  RasterPoint spot_min_pt;
  short spot_max_val;
  short spot_min_val;

protected:
  const ColorRamp *last_color_ramp;

private:
  const RasterTerrain *terrain;

protected:
  bool SlopeShading;
  short TerrainRamp;
  short TerrainContrast;
  short TerrainBrightness;

  void ScanSpotHeights();

  void CopyTo(Canvas &canvas, unsigned width, unsigned height);

  virtual bool do_scan_spot();

public:
  void
  SetSettings(bool _SlopeShading,
              short _TerrainRamp, short _TerrainContrast,
              short _TerrainBrightness)
  {
    SlopeShading = _SlopeShading;
    TerrainRamp = _TerrainRamp;
    TerrainContrast = _TerrainContrast;
    TerrainBrightness = _TerrainBrightness;
  }

  virtual void Draw(Canvas &canvas, const WindowProjection &map_projection,
                    const Angle sunazimuth);
};

#endif
