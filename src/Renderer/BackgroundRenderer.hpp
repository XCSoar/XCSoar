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

#ifndef XCSOAR_BACKGROUND_RENDERER_HPP
#define XCSOAR_BACKGROUND_RENDERER_HPP

#include "Math/Angle.hpp"

#include <memory>

class Canvas;
class WindowProjection;
struct TerrainRendererSettings;
class TerrainRenderer;
class RasterTerrain;
struct DerivedInfo;

/**
 * Utility class to draw terrain, topography (not implemented yet)
 */
class BackgroundRenderer {
  static const Angle DEFAULT_SHADING_ANGLE;

  const RasterTerrain *terrain = nullptr;
  std::unique_ptr<TerrainRenderer> renderer;
  Angle shading_angle = DEFAULT_SHADING_ANGLE;

public:
  BackgroundRenderer();

  ~BackgroundRenderer();

  /**
   * Flush all caches.
   */
  void Flush();

  void Draw(Canvas& canvas,
            const WindowProjection& proj,
            const TerrainRendererSettings &terrain_settings);

  void SetShadingAngle(const WindowProjection &projection,
                       const TerrainRendererSettings &settings,
                       const DerivedInfo &calculated);
  void SetTerrain(const RasterTerrain *terrain);

private:
  void SetShadingAngle(const WindowProjection& proj, Angle angle);
};

#endif
