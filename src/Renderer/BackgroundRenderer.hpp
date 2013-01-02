/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/Point.hpp"
#include "Math/Angle.hpp"

#include <tchar.h>

class Canvas;
class WindowProjection;
struct TerrainRendererSettings;
class TerrainRenderer;
struct SpeedVector;
class RasterTerrain;
class RasterWeather;
class LabelBlock;
struct NMEAInfo;
struct DerivedInfo;

/**
 * Utility class to draw terrain, topography (not implemented yet)
 */
class BackgroundRenderer {
  const RasterTerrain *terrain;
  const RasterWeather *weather;
  TerrainRenderer *renderer;
  Angle shading_angle;

public:
  BackgroundRenderer();
  ~BackgroundRenderer();

  void Draw(Canvas& canvas,
            const WindowProjection& proj,
            const TerrainRendererSettings &terrain_settings);

  bool DrawSpotHeights(Canvas& canvas, LabelBlock& block);

  void SetShadingAngle(const WindowProjection &projection,
                       const TerrainRendererSettings &settings,
                       const DerivedInfo &calculated);
  void Reset();
  void SetTerrain(const RasterTerrain *terrain);
  void SetWeather(const RasterWeather *weather);

private:
  void SetShadingAngle(const WindowProjection& proj, Angle angle);
  static void DrawSpotHeight(Canvas &canvas,  LabelBlock &block,
                             const TCHAR *Buffer, RasterPoint pt);
};

#endif
