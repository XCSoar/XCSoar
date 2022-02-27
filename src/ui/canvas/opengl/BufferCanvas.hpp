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

#ifndef XCSOAR_SCREEN_OPENGL_BUFFER_CANVAS_HPP
#define XCSOAR_SCREEN_OPENGL_BUFFER_CANVAS_HPP

#include "Canvas.hpp"
#include "Math/Point2D.hpp"
#include "ui/opengl/Features.hpp" // for SOFTWARE_ROTATE_DISPLAY

#include <glm/mat4x4.hpp>

#ifdef SOFTWARE_ROTATE_DISPLAY
#include <cstdint>
enum class DisplayOrientation : uint8_t;
#endif

class GLTexture;
class GLFrameBuffer;
class GLRenderBuffer;

/**
 * An off-screen #Canvas implementation.
 */
class BufferCanvas : public Canvas {
  static constexpr GLint INTERNAL_FORMAT = GL_RGB;
  static constexpr GLint FORMAT = GL_RGB;
  static constexpr GLint TYPE = GL_UNSIGNED_BYTE;

  GLTexture *texture = nullptr;

  GLFrameBuffer *frame_buffer = nullptr;

  GLRenderBuffer *stencil_buffer = nullptr;

  GLint old_viewport[4];

  glm::mat4 old_projection_matrix;

  PixelPoint old_translate;
  UnsignedPoint2D old_size;

#ifdef SOFTWARE_ROTATE_DISPLAY
  DisplayOrientation old_orientation;
#endif

#ifndef NDEBUG
  bool active = false;
#endif

public:
  ~BufferCanvas() {
    Destroy();
  }

  bool IsDefined() const {
    return texture != nullptr;
  }

  void Create(PixelSize new_size);

  void Create(const Canvas &canvas, PixelSize new_size) {
    assert(canvas.IsDefined());

    Create(new_size);
  }

  void Create(const Canvas &canvas) {
    Create(canvas, canvas.GetSize());
  }

  void Destroy();

  void Resize(PixelSize new_size);

  /**
   * Similar to Resize(), but never shrinks the buffer.
   */
  void Grow(PixelSize new_size);

  /**
   * Begin painting to the buffer and to the specified #Canvas.
   *
   * @param other an on-screen #Canvas
   */
  void Begin(Canvas &other);

  /**
   * Commit the data that was painted into this #BufferCanvas into
   * both the buffer and the other #Canvas.
   *
   * This method must be called before the next Begin().  There is no
   * rollback method, and painting to the buffer may be destructive
   * for the "other" #Canvas until Commit() is called.
   *
   * @param other an on-screen #Canvas
   */
  void Commit(Canvas &other);

  void CopyTo(Canvas &other);
};

#endif
