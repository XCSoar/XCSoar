/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "MapDrawHelper.hpp"
#include "Screen/Canvas.hpp"
#include "WindowProjection.hpp"
#include "SettingsMap.hpp"
#include "MapCanvas.hpp"

MapDrawHelper::MapDrawHelper(Canvas &_canvas, 
#ifndef ENABLE_OPENGL
                             Canvas &_buffer, 
                             Canvas &_stencil, 
#endif
                             const Projection &_proj,
                             const SETTINGS_MAP& settings_map):
  m_canvas(_canvas),
#ifndef ENABLE_OPENGL
  m_buffer(_buffer),
  m_stencil(_stencil),
#else
  m_buffer(_canvas),
#endif
  m_proj(_proj),
#ifndef ENABLE_OPENGL
  m_buffer_drawn(false),
  m_use_stencil(false),
#endif
  m_settings_map(settings_map)
{
}

MapDrawHelper::MapDrawHelper(MapDrawHelper &_that):
  m_canvas(_that.m_canvas),
#ifndef ENABLE_OPENGL
  m_buffer(_that.m_buffer),
  m_stencil(_that.m_stencil),
#else
  m_buffer(_that.m_canvas),
#endif
  m_proj(_that.m_proj),
#ifndef ENABLE_OPENGL
  m_buffer_drawn(_that.m_buffer_drawn),
  m_use_stencil(_that.m_use_stencil),
#endif
  m_settings_map(_that.m_settings_map)
{
}

void 
MapDrawHelper::draw_search_point_vector(Canvas& the_canvas, 
                                        const SearchPointVector& points) 
{
  const size_t size = points.size();
  if (size<3) {
    return;
  }

  MapCanvas map_canvas(the_canvas, m_proj);
  RasterPoint screen[size];
  map_canvas.project(points, screen);

  if (!map_canvas.visible(screen, size))
    return;

  the_canvas.polygon(&screen[0], size);
#ifndef ENABLE_OPENGL
  if (m_use_stencil) {
    m_stencil.polygon(&screen[0], size);
  }
#endif
}

void 
MapDrawHelper::draw_circle(Canvas &the_canvas,
                           const RasterPoint &center, unsigned radius)
{
  the_canvas.circle(center.x, center.y, radius);
#ifndef ENABLE_OPENGL
  if (m_use_stencil) {
    m_stencil.circle(center.x, center.y, radius);
  }
#endif
}

void 
MapDrawHelper::buffer_render_finish() 
{
#ifndef ENABLE_OPENGL
  if (m_buffer_drawn) {
    // need to do this to prevent drawing of colored outline
    m_buffer.white_pen();
    
    if (m_use_stencil) {
      m_buffer.copy_transparent_black(m_stencil);
    }
    m_canvas.copy_and(m_buffer);
    m_buffer_drawn = false;
  }
#endif
}

void 
MapDrawHelper::buffer_render_start() 
{
#ifndef ENABLE_OPENGL
  if (!m_buffer_drawn) {
    clear_buffer();
    m_buffer_drawn = true;
  }
#endif
}

void 
MapDrawHelper::clear_buffer() {
#ifndef ENABLE_OPENGL
  m_buffer.clear_white();
  m_stencil.clear_white();
#endif
}
