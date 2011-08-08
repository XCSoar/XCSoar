/*
Copyright_License {

  XCSoar Glide Compute5r - http://www.xcsoar.org/
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

#ifndef XCSOAR_SCREEN_OPENGL_BUFFER_HPP
#define XCSOAR_SCREEN_OPENGL_BUFFER_HPP

#include "Screen/OpenGL/Features.hpp"

#include <assert.h>

#ifdef HAVE_GLES
#include <GLES/gl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif

#ifndef NDEBUG
extern unsigned num_buffers;
#endif

/**
 * This class represents an OpenGL buffer object.
 */
class GLBuffer {
  GLuint id;

public:
  GLBuffer() {
    glGenBuffers(1, &id);

#ifndef NDEBUG
    ++num_buffers;
#endif
  }

  explicit GLBuffer(GLuint _id)
    :id(_id) {
#ifndef NDEBUG
    ++num_buffers;
#endif
  }

  ~GLBuffer() {
#ifndef NDEBUG
    assert(num_buffers > 0);
    --num_buffers;
#endif

    glDeleteBuffers(1, &id);
  }

  void Bind(GLenum target) {
    glBindBuffer(target, id);
  }

  static void Unbind(GLenum target) {
    glBindBuffer(target, 0);
  }

  void Load(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) {
    Bind(target);
    glBufferData(target, size, data, usage);
    Unbind(target);
  }
};

class GLArrayBuffer : private GLBuffer {
public:
  GLArrayBuffer() {}
  explicit GLArrayBuffer(GLuint _id):GLBuffer(_id) {}

  void Bind() {
    GLBuffer::Bind(GL_ARRAY_BUFFER);
  }

  static void Unbind() {
    GLBuffer::Unbind(GL_ARRAY_BUFFER);
  }

  void Load(GLsizeiptr size, const GLvoid *data) {
    GLBuffer::Load(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
  }
};

#endif
