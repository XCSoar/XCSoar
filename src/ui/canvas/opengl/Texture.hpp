// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/System.hpp"
#include "ui/dim/Rect.hpp"
#include "FBO.hpp"

#include <cassert>

/**
 * This class represents an OpenGL texture.
 */
class GLTexture {
protected:
  GLuint id;

  PixelSize size;

  /**
   * The real dimensions of the texture.  This may differ when
   * ARB_texture_non_power_of_two is not available.
   */
  PixelSize allocated_size;

  /** Texture may be vertically flipped. */
  bool flipped;

public:
#ifdef ANDROID
  GLTexture(GLuint _id, PixelSize _size, PixelSize _allocated_size,
            bool _flipped = false) noexcept
    :id(_id), size(_size), allocated_size(_allocated_size), flipped(_flipped) {
#ifndef NDEBUG
    assert(allocated_size.width >= size.width);
    assert(allocated_size.height >= size.height);
#endif
  }
#endif

  /**
   * Create a texture with undefined content.
   */
  explicit GLTexture(GLint internal_format, PixelSize _size,
                     GLenum format, GLenum type,
                     bool _flipped = false) noexcept;

  GLTexture(GLint internal_format, PixelSize _size,
            GLenum format, GLenum type, const GLvoid *data,
            bool _flipped = false) noexcept;

  ~GLTexture() noexcept {
    glDeleteTextures(1, &id);
  }

  unsigned GetWidth() const noexcept {
    return size.width;
  }

  unsigned GetHeight() const noexcept {
    return size.height;
  }

  [[gnu::pure]]
  const PixelSize &GetSize() const noexcept {
    return size;
  }

  [[gnu::pure]]
  PixelRect GetRect() const noexcept {
    return PixelRect(GetSize());
  }

  /**
   * Returns the physical size of the texture.
   */
  [[gnu::pure]]
  const PixelSize &GetAllocatedSize() const noexcept {
    return allocated_size;
  }

  bool IsFlipped() const noexcept {
    return flipped;
  }

  /**
   * Change the size of the texture, discarding any previous contents.
   */
  void ResizeDiscard(GLint internal_format, PixelSize new_size,
                     GLenum format, GLenum type) noexcept;

protected:
  void Initialise() noexcept;

  static void Configure() noexcept;

public:
  void Bind() noexcept {
    glBindTexture(GL_TEXTURE_2D, id);
  }

  void AttachFramebuffer(GLenum attachment) noexcept {
    FBO::FramebufferTexture2D(FBO::FRAMEBUFFER, attachment,
                              GL_TEXTURE_2D, id, 0);
  }

  void Draw(PixelRect dest, PixelRect src) const noexcept;

  void Draw(PixelPoint dest) const noexcept {
    Draw(PixelRect(dest, GetSize()), GetRect());
  }
};
