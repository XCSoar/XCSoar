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

#ifndef XCSOAR_SCREEN_OPENGL_TYPES_HPP
#define XCSOAR_SCREEN_OPENGL_TYPES_HPP

#include "Features.hpp"
#include "System.hpp"

#ifdef HAVE_GLES

/**
 * A position component in the OpenGL surface.
 */
typedef GLshort GLvalue;
typedef GLushort GLuvalue;
static constexpr GLenum GL_VALUE = GL_SHORT;

typedef GLfixed GLexact;
static constexpr GLenum GL_EXACT = GL_FIXED;

constexpr static inline GLexact
ToGLexact(GLvalue value)
{
  return (GLexact(value) << 16) + 0x8000;
}

#else

/**
 * A position component in the OpenGL surface.
 */
typedef GLint GLvalue;
typedef GLuint GLuvalue;
static constexpr GLenum GL_VALUE = GL_INT;

typedef GLfloat GLexact;
static constexpr GLenum GL_EXACT = GL_FLOAT;

constexpr static inline GLexact
ToGLexact(GLvalue value)
{
  return GLexact(value) + 0.5;
}

#endif

#endif
