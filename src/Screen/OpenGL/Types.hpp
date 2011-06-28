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

#ifndef XCSOAR_SCREEN_OPENGL_TYPES_HPP
#define XCSOAR_SCREEN_OPENGL_TYPES_HPP

#include "Screen/OpenGL/Features.hpp"

#ifdef HAVE_GLES

#include <GLES/gl.h>

/**
 * A position component in the OpenGL surface.
 */
typedef GLshort GLvalue;
static const GLenum GL_VALUE = GL_SHORT;

#else

#include <SDL/SDL_opengl.h>

/**
 * A position component in the OpenGL surface.
 */
typedef GLint GLvalue;
static const GLenum GL_VALUE = GL_INT;

#endif

#endif
