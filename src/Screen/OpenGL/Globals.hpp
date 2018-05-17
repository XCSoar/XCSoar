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

/*
 * A collection of global variables for the OpenGL backend.  Global
 * variables are not good style, but since there can be only once
 * OpenGL context at a time, this is good enough for XCSoar.
 *
 */

#ifndef XCSOAR_SCREEN_OPENGL_GLOBALS_HPP
#define XCSOAR_SCREEN_OPENGL_GLOBALS_HPP

#include "Screen/OpenGL/Features.hpp"
#include "System.hpp"

#ifdef USE_GLSL
#include <glm/glm.hpp>
#endif

#ifdef SOFTWARE_ROTATE_DISPLAY
#include <stdint.h>
enum class DisplayOrientation : uint8_t;
#endif

struct UnsignedPoint2D;
struct PixelPoint;

namespace OpenGL {
  /**
   * Is the extension ARB_texture_non_power_of_two present?  If yes,
   * then textures can have any size, not just power of two.
   */
  extern bool texture_non_power_of_two;

#ifdef HAVE_OES_DRAW_TEXTURE
  /**
   * Shall we use the OES_draw_texture extension?
   */
  extern bool oes_draw_texture;
#endif

  /**
   * Is it safe to use VBO?
   *
   * This check is important, because Android 1.6 will happily crash
   * upon attempting to delete a VBO, a bug that will probably never
   * get fixed (report is nearly 2 years old at the time of this
   * writing):
   * http://code.google.com/p/android/issues/detail?id=4273
   */
#ifdef ANDROID
  extern bool vertex_buffer_object;
#else
  static constexpr bool vertex_buffer_object = true;
#endif

  /**
   * Is glMapBuffer() available?  May be implemented by the extension
   * GL_OES_mapbuffer.
   */
#ifdef HAVE_OES_MAPBUFFER
  extern bool mapbuffer;
#else
  static constexpr bool mapbuffer = true;
#endif

  /**
   * Is the extension GL_ARB_framebuffer_object or
   * GL_OES_framebuffer_object present?
   */
  extern bool frame_buffer_object;

  /**
   * Which depth+stencil internalFormat is supported by the
   * Renderbuffer?  This is only set if frame_buffer_object is true.
   */
  extern GLenum render_buffer_depth_stencil;

  /**
   * Which stencil internalFormat is supported by the Renderbuffer?
   * This is only set if frame_buffer_object is true.
   */
  extern GLenum render_buffer_stencil;

  /**
   * The dimensions of the OpenGL window in pixels.
   */
  extern UnsignedPoint2D window_size;

  /**
   * The dimensions of the OpenGL viewport in pixels.
   */
  extern UnsignedPoint2D viewport_size;

#ifdef SOFTWARE_ROTATE_DISPLAY
  extern DisplayOrientation display_orientation;
#endif

  /**
   * The current SubCanvas translation in pixels.
   */
  extern PixelPoint translate;

#ifdef USE_GLSL
  extern glm::mat4 projection_matrix;
#endif
};

#endif
