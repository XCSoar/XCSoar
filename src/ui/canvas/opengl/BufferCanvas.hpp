// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  ~BufferCanvas() noexcept {
    Destroy();
  }

  bool IsDefined() const noexcept {
    return texture != nullptr;
  }

  void Create(PixelSize new_size) noexcept;

  void Create([[maybe_unused]] const Canvas &canvas, PixelSize new_size) noexcept {
    assert(canvas.IsDefined());

    Create(new_size);
  }

  void Create(const Canvas &canvas) noexcept {
    Create(canvas, canvas.GetSize());
  }

  void Destroy() noexcept;

  void Resize(PixelSize new_size) noexcept;

  /**
   * Similar to Resize(), but never shrinks the buffer.
   */
  void Grow(PixelSize new_size) noexcept;

  /**
   * Begin painting to the buffer and to the specified #Canvas.
   *
   * @param other an on-screen #Canvas
   */
  void Begin(Canvas &other) noexcept;

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
  void Commit(Canvas &other) noexcept;

  void CopyTo(Canvas &other) noexcept;
};
