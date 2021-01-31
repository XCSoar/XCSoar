/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Globals.hpp"
#include "Debug.hpp"
#include "ui/dim/Point.hpp"

#include <glm/mat4x4.hpp>

namespace OpenGL {
  bool texture_non_power_of_two;

#ifdef HAVE_OES_DRAW_TEXTURE
  bool oes_draw_texture;
#endif

#ifdef HAVE_OES_MAPBUFFER
  bool mapbuffer;
#endif

  bool frame_buffer_object;

  GLenum render_buffer_depth_stencil, render_buffer_stencil;

  UnsignedPoint2D window_size, viewport_size;

#ifdef SOFTWARE_ROTATE_DISPLAY
  DisplayOrientation display_orientation;
#endif

  PixelPoint translate;

  glm::mat4 projection_matrix;

#ifndef NDEBUG
  pthread_t thread;
#endif
};
