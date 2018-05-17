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

#ifndef XCSOAR_SCREEN_OPENGL_FALLBACK_BUFFER_HPP
#define XCSOAR_SCREEN_OPENGL_FALLBACK_BUFFER_HPP

#include "Buffer.hpp"
#include "Util/Manual.hxx"
#include "Util/WritableBuffer.hxx"

#include <assert.h>
#include <stdlib.h>

/**
 * This class represents an OpenGL buffer object with a fallback to
 * heap-allocated buffer.
 */
template<class B>
class GLFallbackBuffer {
  Manual<B> vbo;

  /**
   * An allocated fallback buffer if VBO is not available.
   */
  WritableBuffer<void> fallback;

public:
  GLFallbackBuffer() {
    if (OpenGL::vertex_buffer_object) {
      vbo.Construct();
    } else {
      fallback = nullptr;
    }
  }

  ~GLFallbackBuffer() {
    if (OpenGL::vertex_buffer_object) {
      vbo.Destruct();
    } else {
      free(fallback.data);
    }
  }

  GLvoid *BeginWrite(size_t size) {
    if (OpenGL::vertex_buffer_object) {
      return vbo->BeginWrite(size);
    } else {
      if (size != fallback.size) {
        free(fallback.data);
        fallback.data = malloc(size);
        fallback.size = size;
      }

      return fallback.data;
    }
  }

  void CommitWrite(size_t size, GLvoid *data) {
    if (OpenGL::vertex_buffer_object) {
      vbo->CommitWrite(size, data);
    } else {
      assert(size == fallback.size);
      assert(data == fallback.data);
    }
  }

  const GLvoid *BeginRead() {
    if (OpenGL::vertex_buffer_object) {
      vbo->Bind();
      /* pointer relative to the VBO */
      return nullptr;
    } else {
      /* regular pointer */
      return fallback.data;
    }
  }

  void EndRead() {
    if (OpenGL::vertex_buffer_object)
      vbo->Unbind();
  }
};

class GLFallbackArrayBuffer : public GLFallbackBuffer<GLArrayBuffer> {
};

#endif
