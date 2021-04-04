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

#ifndef XCSOAR_SCREEN_OPENGL_VERTEX_POINTER_HPP
#define XCSOAR_SCREEN_OPENGL_VERTEX_POINTER_HPP

#include "ui/opengl/System.hpp"
#include "ui/opengl/Types.hpp"
#include "Attribute.hpp"

struct FloatPoint2D;
struct BulkPixelPoint;
struct ExactPixelPoint;

struct ScopeVertexPointer {
  ScopeVertexPointer() noexcept {
    glEnableVertexAttribArray(OpenGL::Attribute::POSITION);
  }

  ~ScopeVertexPointer() noexcept {
    glDisableVertexAttribArray(OpenGL::Attribute::POSITION);
  }

  ScopeVertexPointer(GLenum type, const void *p) noexcept {
    glEnableVertexAttribArray(OpenGL::Attribute::POSITION);
    Update(type, p);
  }

  template<typename T>
  ScopeVertexPointer(const T *p) noexcept {
    glEnableVertexAttribArray(OpenGL::Attribute::POSITION);
    Update(p);
  }

  void Update(GLenum type, GLsizei stride, const void *p) noexcept {
    glVertexAttribPointer(OpenGL::Attribute::POSITION, 2, type,
                          GL_FALSE, stride, p);
  }

  void Update(GLenum type, const void *p) noexcept {
    Update(type, 0, p);
  }

  void Update(const BulkPixelPoint *p) noexcept {
    Update(GL_VALUE, p);
  }

  void Update(const ExactPixelPoint *p) noexcept {
    Update(GL_EXACT, p);
  }

  void Update(const FloatPoint2D *p) noexcept {
    Update(GL_FLOAT, p);
  }
};

#endif
