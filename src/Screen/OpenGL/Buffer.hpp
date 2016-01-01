/*
Copyright_License {

  XCSoar Glide Compute5r - http://www.xcsoar.org/
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

#ifndef XCSOAR_SCREEN_OPENGL_BUFFER_HPP
#define XCSOAR_SCREEN_OPENGL_BUFFER_HPP

#include "SystemExt.hpp"
#include "Globals.hpp"
#include "Features.hpp"

#ifdef HAVE_DYNAMIC_MAPBUFFER
#include "Dynamic.hpp"
#endif

#include <assert.h>
#include <stdlib.h>

#ifndef NDEBUG
extern unsigned num_buffers;
#endif

/**
 * This class represents an OpenGL buffer object.
 */
template<GLenum target, GLenum usage>
class GLBuffer {
  GLuint id;

#ifndef NDEBUG
  GLvoid *p;
#endif

public:
  GLBuffer() {
    glGenBuffers(1, &id);

#ifndef NDEBUG
    p = nullptr;
    ++num_buffers;
#endif
  }

  GLBuffer(const GLBuffer &) = delete;

  ~GLBuffer() {
#ifndef NDEBUG
    assert(p == nullptr);
    assert(num_buffers > 0);
    --num_buffers;
#endif

    glDeleteBuffers(1, &id);
  }

  void Bind() {
    assert(p == nullptr);

    glBindBuffer(target, id);
  }

  static void Unbind() {
    glBindBuffer(target, 0);
  }

  /**
   * Allocates and initializes the buffer.
   */
  static void Data(GLsizeiptr size, const GLvoid *data) {
    glBufferData(target, size, data, usage);
  }

  void Load(GLsizeiptr size, const GLvoid *data) {
    Bind();
    Data(size, data);
    Unbind();
  }

  static void *MapWrite() {
#ifdef HAVE_DYNAMIC_MAPBUFFER
    return GLExt::map_buffer(target, GL_WRITE_ONLY_OES);
#elif defined(GL_OES_mapbuffer)
    return glMapBufferOES(target, GL_WRITE_ONLY_OES);
#else
    return glMapBuffer(target, GL_WRITE_ONLY);
#endif
  }

  static void Unmap() {
#ifdef HAVE_DYNAMIC_MAPBUFFER
    GLExt::unmap_buffer(target);
#elif defined(GL_OES_mapbuffer)
    glUnmapBufferOES(target);
#else
    glUnmapBuffer(target);
#endif
  }

  GLvoid *BeginWrite(size_t size) {
    Bind();

    void *result;
    if (OpenGL::mapbuffer) {
      Data(GLsizeiptr(size), nullptr);
      result = MapWrite();
    } else {
      result = malloc(size);
    }

#ifndef NDEBUG
    p = result;
#endif

    return result;
  }

  void CommitWrite(size_t size, GLvoid *data) {
#ifndef NDEBUG
    assert(data == p);
    p = nullptr;
#endif

    if (OpenGL::mapbuffer) {
      Unmap();
    } else {
      Data(GLsizeiptr(size), data);
      free(data);
    }

    Unbind();
  }
};

class GLArrayBuffer : public GLBuffer<GL_ARRAY_BUFFER, GL_STATIC_DRAW> {
};

#endif
