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

#ifndef XCSOAR_SCREEN_OPENGL_TEXTURE_HPP
#define XCSOAR_SCREEN_OPENGL_TEXTURE_HPP

#include "ui/opengl/Features.hpp"
#include "ui/opengl/System.hpp"
#include "ui/dim/Rect.hpp"
#include "FBO.hpp"
#include "Asset.hpp"

#include <cassert>

#ifdef ENABLE_SDL
#include <SDL_video.h>
#endif

#ifndef NDEBUG
extern unsigned num_textures;
#endif

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

    ++num_textures;
#endif
  }
#endif

  /**
   * Create a texture with undefined content.
   */
  explicit GLTexture(PixelSize _size, bool _flipped = false) noexcept;

  GLTexture(GLint internal_format, PixelSize _size,
            GLenum format, GLenum type, const GLvoid *data,
            bool _flipped = false) noexcept;

  ~GLTexture() noexcept {
    glDeleteTextures(1, &id);

#ifndef NDEBUG
    assert(num_textures > 0);
    --num_textures;
#endif
  }

  /**
   * Returns the standard pixel format of the platform.
   */
  static constexpr GLenum GetType() noexcept {
    return HaveGLES()
      ? GL_UNSIGNED_SHORT_5_6_5
      : GL_UNSIGNED_BYTE;
  }

  unsigned GetWidth() const noexcept {
    return size.width;
  }

  unsigned GetHeight() const noexcept {
    return size.height;
  }

  gcc_pure
  const PixelSize &GetSize() const noexcept {
    return size;
  }

  gcc_pure
  PixelRect GetRect() const noexcept {
    return PixelRect(GetSize());
  }

  /**
   * Returns the physical size of the texture.
   */
  gcc_pure
  const PixelSize &GetAllocatedSize() const noexcept {
    return allocated_size;
  }

  bool IsFlipped() const noexcept {
    return flipped;
  }

  /**
   * Enable interpolation when minifying/magnifying the texture.  The
   * caller must bind the texture prior to calling this method.
   */
  static void EnableInterpolation() noexcept;

  /**
   * Change the size of the texture, discarding any previous contents.
   */
  void ResizeDiscard(PixelSize new_size) noexcept;

protected:
  void Initialise() noexcept;

  static void Configure() noexcept;

#ifdef HAVE_OES_DRAW_TEXTURE
private:
  void DrawOES(PixelRect dest, PixelRect src) const noexcept;
#endif

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

#endif
