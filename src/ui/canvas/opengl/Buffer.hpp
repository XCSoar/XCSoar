// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/SystemExt.hpp"
#include "ui/opengl/Features.hpp"
#include "Globals.hpp"

#ifdef HAVE_DYNAMIC_MAPBUFFER
#include "Dynamic.hpp"
#endif

#include <cassert>
#include <stdlib.h>

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
  GLBuffer() noexcept {
    glGenBuffers(1, &id);

#ifndef NDEBUG
    p = nullptr;
#endif
  }

  GLBuffer(const GLBuffer &) = delete;
  GLBuffer &operator=(const GLBuffer &) = delete;

  ~GLBuffer() noexcept {
#ifndef NDEBUG
    assert(p == nullptr);
#endif

    glDeleteBuffers(1, &id);
  }

  void Bind() noexcept {
    assert(p == nullptr);

    glBindBuffer(target, id);
  }

  static void Unbind() noexcept {
    glBindBuffer(target, 0);
  }

  /**
   * Allocates and initializes the buffer.
   */
  static void Data(GLsizeiptr size, const GLvoid *data) noexcept {
    glBufferData(target, size, data, usage);
  }

  void Load(GLsizeiptr size, const GLvoid *data) noexcept {
    Bind();
    Data(size, data);
    Unbind();
  }

  static void *MapWrite() noexcept {
#ifdef HAVE_DYNAMIC_MAPBUFFER
    return GLExt::map_buffer(target, GL_WRITE_ONLY_OES);
#elif defined(GL_OES_mapbuffer)
    return glMapBufferOES(target, GL_WRITE_ONLY_OES);
#else
    return glMapBuffer(target, GL_WRITE_ONLY);
#endif
  }

  static void Unmap() noexcept {
#ifdef HAVE_DYNAMIC_MAPBUFFER
    GLExt::unmap_buffer(target);
#elif defined(GL_OES_mapbuffer)
    glUnmapBufferOES(target);
#else
    glUnmapBuffer(target);
#endif
  }

  GLvoid *BeginWrite(size_t size) noexcept {
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

  void CommitWrite(size_t size, GLvoid *data) noexcept {
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
