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

#ifndef XCSOAR_SCREEN_OPENGL_SCOPE_HPP
#define XCSOAR_SCREEN_OPENGL_SCOPE_HPP

#include "Features.hpp"
#include "System.hpp"

/**
 * Enables and auto-disables an OpenGL capability.
 */
template<GLenum cap>
class GLEnable {
public:
  GLEnable() {
    ::glEnable(cap);
  }

  ~GLEnable() {
    ::glDisable(cap);
  }
};

class GLBlend : public GLEnable<GL_BLEND> {
public:
  GLBlend(GLenum sfactor, GLenum dfactor) {
    ::glBlendFunc(sfactor, dfactor);
  }

#ifndef HAVE_GLES1
  GLBlend(GLclampf alpha) {
    ::glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
    ::glBlendColor(0, 0, 0, alpha);
  }
#endif
};

/**
 * Enable alpha blending with source's alpha value (the most common
 * variant of GL_BLEND).
 */
class ScopeAlphaBlend : GLBlend {
public:
  ScopeAlphaBlend():GLBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) {}
};

class GLScissor : public GLEnable<GL_SCISSOR_TEST> {
public:
  GLScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    ::glScissor(x, y, width, height);
  }
};

#endif
