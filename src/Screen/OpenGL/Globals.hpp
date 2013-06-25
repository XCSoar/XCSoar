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

/*
 * A collection of global variables for the OpenGL backend.  Global
 * variables are not good style, but since there can be only once
 * OpenGL context at a time, this is good enough for XCSoar.
 *
 */

#ifndef XCSOAR_SCREEN_OPENGL_GLOBALS_HPP
#define XCSOAR_SCREEN_OPENGL_GLOBALS_HPP

#include "Screen/OpenGL/Point.hpp"
#include "Screen/OpenGL/Features.hpp"

#ifndef NDEBUG
#include <pthread.h>
#endif

namespace OpenGL {
#ifdef HAVE_DYNAMIC_EGL
  /**
   * Was EGL detected at runtime?  The EGL API has been supported
   * since Android API level 9, but we require API level 4, so we need
   * to detect at runtime.
   */
  extern bool egl;
#endif

  /**
   * Is the extension ARB_texture_non_power_of_two present?  If yes,
   * then textures can have any size, not just power of two.
   */
  extern bool texture_non_power_of_two;

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
   * The dimensions of the screen in pixels.
   */
  extern UPixelScalar screen_width, screen_height;

  /**
   * The current SubCanvas translation in pixels.
   */
  extern RasterPoint translate;
};

#endif
