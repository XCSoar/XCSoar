// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "System.hpp"

/**
 * A position component in the OpenGL surface.
 */
typedef GLshort GLvalue;
typedef GLushort GLuvalue;
static constexpr GLenum GL_VALUE = GL_SHORT;

#ifdef HAVE_GLES2
// OpenGL ES 2.0 uses fixed-point math
typedef GLfixed GLexact;
static constexpr GLenum GL_EXACT = GL_FIXED;

constexpr static inline GLexact
ToGLexact(GLvalue value)
{
  return (GLexact(value) << 16) + 0x8000;
}
#else
// Desktop OpenGL doesn't have GL_FIXED type, use GLfloat instead
typedef GLfloat GLexact;
static constexpr GLenum GL_EXACT = GL_FLOAT;

constexpr static inline GLexact
ToGLexact(GLvalue value)
{
  return GLexact(value) + 0.5f;
}
#endif
