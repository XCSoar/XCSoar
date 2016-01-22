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

#ifndef XCSOAR_TERRAIN_RENDERER_HPP
#define XCSOAR_TERRAIN_RENDERER_HPP

#include "RasterRenderer.hpp"
#include "Util/Serial.hpp"
#include "Terrain/TerrainSettings.hpp"

#ifndef ENABLE_OPENGL
#include "Projection/CompareProjection.hpp"
#endif

class Canvas;
class WindowProjection;
class RasterTerrain;
struct ColorRamp;

class TerrainRenderer {
  const RasterTerrain &terrain;

  Serial terrain_serial;

protected:
  struct TerrainRendererSettings settings;

#ifndef ENABLE_OPENGL
  CompareProjection compare_projection;
#endif

  Angle last_sun_azimuth = Angle::Zero();

  const ColorRamp *last_color_ramp = nullptr;

  RasterRenderer raster_renderer;

public:
  TerrainRenderer(const RasterTerrain &_terrain);
  ~TerrainRenderer() {}

  TerrainRenderer(const TerrainRenderer &) = delete;
  TerrainRenderer &operator=(const TerrainRenderer &) = delete;

  /**
   * Flush the cache.
   */
  void Flush() {
#ifdef ENABLE_OPENGL
    raster_renderer.Invalidate();
#else
    compare_projection.Clear();
#endif
  }

public:
  const TerrainRendererSettings &GetSettings() const {
    return settings;
  }

  void SetSettings(const TerrainRendererSettings &_settings) {
    settings = _settings;
  }

  /**
   * @return true if an image has been renderered and Draw() may be
   * called
   */
  bool Generate(const WindowProjection &map_projection,
                const Angle sunazimuth);

  void Draw(Canvas &canvas, const WindowProjection &projection) const {
    raster_renderer.Draw(canvas, projection);
  }
};

#endif
