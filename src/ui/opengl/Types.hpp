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

typedef GLfixed GLexact;
static constexpr GLenum GL_EXACT = GL_FIXED;

constexpr static inline GLexact
ToGLexact(GLvalue value)
{
  return (GLexact(value) << 16) + 0x8000;
}
