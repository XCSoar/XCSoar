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

#include "Navigation/SearchPointVector.hpp"
#include "Screen/Point.hpp"

class Canvas;
class Projection;
struct SETTINGS_MAP;

/**
 * Utility class to draw multilayer items on a canvas with stencil masking
 */
class MapDrawHelper 
{
public:
  MapDrawHelper(Canvas &_canvas, 
#ifndef ENABLE_OPENGL
                Canvas &_buffer, 
                Canvas &_stencil, 
#endif
                const Projection &_proj,
                const SETTINGS_MAP& settings_map);

  MapDrawHelper(MapDrawHelper &_that);

  Canvas &m_canvas;
  Canvas &m_buffer;
#ifndef ENABLE_OPENGL
  Canvas &m_stencil;
#endif
  const Projection& m_proj;
#ifndef ENABLE_OPENGL
  bool m_buffer_drawn;
  bool m_use_stencil;
#endif

  const SETTINGS_MAP& m_settings_map;

protected:

  void draw_search_point_vector(Canvas& the_canvas, const SearchPointVector& points);

  void draw_circle(Canvas &the_canvas,
                   const RasterPoint &center, unsigned radius);

  void buffer_render_finish();

  void buffer_render_start();

  void clear_buffer();
};


#endif
