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

#ifndef XCSOAR_SCREEN_OPENGL_VERTEX_POINTER_HPP
#define XCSOAR_SCREEN_OPENGL_VERTEX_POINTER_HPP

#include "System.hpp"
#include "Types.hpp"

#ifdef USE_GLSL
#include "Attribute.hpp"
#endif

struct FloatPoint2D;
struct BulkPixelPoint;
struct ExactPixelPoint;

struct ScopeVertexPointer {
#ifdef USE_GLSL
  ScopeVertexPointer() {
    glEnableVertexAttribArray(OpenGL::Attribute::POSITION);
  }

  ~ScopeVertexPointer() {
    glDisableVertexAttribArray(OpenGL::Attribute::POSITION);
  }
#else
  ScopeVertexPointer() = default;
#endif

  ScopeVertexPointer(GLenum type, const void *p) {
#ifdef USE_GLSL
    glEnableVertexAttribArray(OpenGL::Attribute::POSITION);
#endif
    Update(type, p);
  }

  template<typename T>
  ScopeVertexPointer(const T *p) {
#ifdef USE_GLSL
    glEnableVertexAttribArray(OpenGL::Attribute::POSITION);
#endif
    Update(p);
  }

  void Update(GLenum type, GLsizei stride, const void *p) {
#ifdef USE_GLSL
    glVertexAttribPointer(OpenGL::Attribute::POSITION, 2, type,
                          GL_FALSE, stride, p);
#else
    glVertexPointer(2, type, stride, p);
#endif
  }

  void Update(GLenum type, const void *p) {
    Update(type, 0, p);
  }

  void Update(const BulkPixelPoint *p) {
    Update(GL_VALUE, p);
  }

  void Update(const ExactPixelPoint *p) {
    Update(GL_EXACT, p);
  }

  void Update(const FloatPoint2D *p) {
    Update(GL_FLOAT, p);
  }
};

#endif
