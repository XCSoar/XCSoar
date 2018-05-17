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

#ifndef XCSOAR_WEATHER_RASP_RENDERER_HPP
#define XCSOAR_WEATHER_RASP_RENDERER_HPP

#include "RaspCache.hpp"
#include "Terrain/RasterRenderer.hpp"
#include "Time/BrokenTime.hpp"

#ifndef ENABLE_OPENGL
#include "Projection/CompareProjection.hpp"
#endif

struct TerrainRendererSettings;

class RaspRenderer {
  RaspCache cache;

  RasterRenderer raster_renderer;

#ifndef ENABLE_OPENGL
  CompareProjection compare_projection;
#endif

  const ColorRamp *last_color_ramp = nullptr;

public:
  RaspRenderer(const RaspStore &_store, unsigned parameter)
    :cache(_store, parameter) {}

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

  unsigned GetParameter() const {
    return cache.GetParameter();
  }

  /**
   * Returns the human-readable name for the current RASP map, or
   * nullptr if no RASP map is enabled.
   */
  gcc_pure
  const TCHAR *GetLabel() const {
    return cache.GetMapLabel();
  }

  gcc_pure
  bool IsInside(GeoPoint p) const {
    return cache.IsInside(p);
  }

  void SetTime(BrokenTime t) {
    cache.SetTime(t);
  }

  void Update(BrokenTime time_local, OperationEnvironment &operation) {
    cache.Reload(time_local, operation);
  }

  /**
   * @return true if an image has been renderered and Draw() may be
   * called
   */
  bool Generate(const WindowProjection &projection,
                const TerrainRendererSettings &settings);

  void Draw(Canvas &canvas, const WindowProjection &projection) const {
    raster_renderer.Draw(canvas, projection, true);
  }
};

#endif
