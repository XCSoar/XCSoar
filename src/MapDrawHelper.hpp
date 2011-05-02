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

#ifndef MAP_DRAW_HELPER_HPP
#define MAP_DRAW_HELPER_HPP

#ifndef ENABLE_OPENGL

#include "Navigation/SearchPointVector.hpp"
#include "Screen/Point.hpp"
#include "Geo/GeoClip.hpp"
#include "Util/AllocatedArray.hpp"

class Canvas;
class Projection;
class WindowProjection;
struct SETTINGS_MAP;

/**
 * Utility class to draw multilayer items on a canvas with stencil masking
 */
class MapDrawHelper
{
  const GeoClip clip;

  /**
   * A variable-length buffer for clipped GeoPoints.
   */
  AllocatedArray<GeoPoint> geo_points;

public:
  Canvas &m_canvas;
  Canvas &m_buffer;
  Canvas &m_stencil;
  const Projection& m_proj;
  bool m_buffer_drawn;
  bool m_use_stencil;

  const SETTINGS_MAP& m_settings_map;

public:
  MapDrawHelper(Canvas &_canvas,
                Canvas &_buffer,
                Canvas &_stencil,
                const WindowProjection &_proj,
                const SETTINGS_MAP& settings_map);

  MapDrawHelper(MapDrawHelper &_that);

protected:
  void draw_search_point_vector(Canvas& the_canvas,
                                const SearchPointVector& points);

  void draw_circle(Canvas &the_canvas,
                   const RasterPoint &center, unsigned radius);

  void buffer_render_finish();

  void buffer_render_start();

  void clear_buffer();
};

#endif // !ENABLE_OPENGL

#endif
