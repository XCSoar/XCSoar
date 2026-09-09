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

  /**
   * The framebuffer #texture is attached to.  With the explicit
   * multisample resolve this is only the resolve target; drawing goes
   * to #msaa_frame_buffer.
   */
  GLFrameBuffer *frame_buffer = nullptr;

  GLRenderBuffer *stencil_buffer = nullptr;

  /**
   * The multisampled framebuffer drawing goes to when the resolve is
   * explicit, together with its multisampled colour attachment.  Both
   * are nullptr otherwise - with the implicit resolve, and when this
   * buffer is not multisampled at all.
   */
  GLFrameBuffer *msaa_frame_buffer = nullptr;
  GLRenderBuffer *color_buffer = nullptr;

  /**
   * The number of MSAA samples this buffer renders with, or 0 for
   * none.  Decided in Create() from the user's setting and what the
   * GL implementation can do, and reset to 0 whenever the
   * multisampled framebuffer turns out to be incomplete (which the
   * new size can cause in Resize(), too).
   */
  GLsizei samples = 0;

  GLint old_viewport[4];

  glm::mat4 old_projection_matrix;

  PixelPoint old_translate;
  UnsignedPoint2D old_size;

  /**
   * Screen-space scissor must not clip FBO draws (e.g. #VScrollPanel
   * leaves GL_SCISSOR_TEST enabled while child OnPaint fills a tall
   * buffer).
   */
  GLboolean old_scissor_enabled = GL_FALSE;

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
   * Begin painting into this buffer's current size (no resize).
   * Pair with #End.  Does not copy to any on-screen canvas.
   */
  void Begin() noexcept;

  /**
   * Begin painting to the buffer, resizing it to match @a other.
   *
   * @param other an on-screen #Canvas
   */
  void Begin(Canvas &other) noexcept;

  /**
   * Finish painting started with #Begin; restores GL state.
   * Does not blit to the screen.
   */
  void End() noexcept;

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

  /**
   * Copy a source rectangle from this buffer onto @a dest.
   */
  void CopyTo(Canvas &dest, PixelRect dest_rc,
              PixelRect src_rc) noexcept;

private:
  /**
   * The framebuffer drawing goes to, which is the multisampled one
   * where there is one.
   */
  [[gnu::pure]]
  GLFrameBuffer &GetDrawFrameBuffer() const noexcept;

  /**
   * Allocate the renderbuffers (and, for the explicit resolve, the
   * second framebuffer) that go with the current #texture and
   * #samples.
   */
  void CreateAttachments() noexcept;

  /**
   * Detach and free what CreateAttachments() allocated.
   */
  void DestroyAttachments() noexcept;

  /**
   * Like CreateAttachments(), but verify that a multisampled
   * framebuffer really is complete, and fall back to no
   * multisampling if it is not.
   */
  void CreateAttachmentsChecked() noexcept;

  /**
   * Bind the draw framebuffer and attach the colour and stencil
   * buffers to it.
   */
  void AttachAll() noexcept;

  /**
   * Finish drawing: resolve the multisamples if that is not implicit,
   * and unbind.
   */
  void FinishDrawing() noexcept;

  void Activate() noexcept;
  void Deactivate() noexcept;
};
