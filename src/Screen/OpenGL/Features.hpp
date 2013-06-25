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

#ifndef XCSOAR_SCREEN_OPENGL_FEATURES_HPP
#define XCSOAR_SCREEN_OPENGL_FEATURES_HPP

#ifndef ENABLE_OPENGL
#error No OpenGL
#endif

#ifdef ANDROID

/**
 * The EGL API is available.  May require a runtime check
 * (OpenGL::egl).
 */
#define HAVE_DYNAMIC_EGL

/**
 * The OES_draw_texture extension is available.
 */
#define HAVE_OES_DRAW_TEXTURE

#endif

#if defined(USE_VIDEOCORE) || defined(HAVE_MALI)
#define DRAW_MOUSE_CURSOR
#endif

/**
 * Running on OpenGL/ES?
 */
constexpr
static inline bool
HaveGLES()
{
#ifdef HAVE_GLES
  return true;
#else
  return false;
#endif
}

#endif
