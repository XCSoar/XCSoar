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

#ifndef MAP_DRAW_HELPER_HPP
#define MAP_DRAW_HELPER_HPP

#ifndef ENABLE_OPENGL

#include "Geo/GeoClip.hpp"
#include "Util/AllocatedArray.hxx"

struct PixelPoint;
class Canvas;
class Projection;
class WindowProjection;
struct AirspaceRendererSettings;
class SearchPointVector;

/**
 * Utility class to draw multilayer items on a canvas with stencil masking
 */
class StencilMapCanvas
{
  const GeoClip clip;

  /**
   * A variable-length buffer for clipped GeoPoints.
   */
  AllocatedArray<GeoPoint> geo_points;

public:
  Canvas &buffer;
  Canvas &stencil;
  const WindowProjection &proj;
  bool buffer_drawn;
  bool use_stencil;

  const AirspaceRendererSettings &settings;

public:
  StencilMapCanvas(Canvas &_buffer,
                   Canvas &_stencil,
                   const WindowProjection &_proj,
                   const AirspaceRendererSettings &_settings);

  StencilMapCanvas(const StencilMapCanvas &other);

  void DrawSearchPointVector(const SearchPointVector &points);

  void DrawCircle(const PixelPoint &center, unsigned radius);

  void Begin();

  /**
   * Commits the calculated results
   *
   * @return true if something has been rendered, false otherwise
   */
  bool Commit();

protected:
  void ClearBuffer();
};

#endif // !ENABLE_OPENGL

#endif
