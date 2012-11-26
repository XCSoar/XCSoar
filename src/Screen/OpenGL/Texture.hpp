/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Features.hpp"
#include "System.hpp"
#include "Screen/OpenGL/Point.hpp"
#include "FBO.hpp"
#include "Asset.hpp"

#include <assert.h>

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
  UPixelScalar width, height;

#ifndef HAVE_OES_DRAW_TEXTURE
  /**
   * The real dimensions of the texture.  This may differ when
   * ARB_texture_non_power_of_two is not available.
   */
  GLsizei allocated_width, allocated_height;
#endif

public:
#ifdef ANDROID
  GLTexture(GLuint _id, UPixelScalar _width, UPixelScalar _height)
    :id(_id), width(_width), height(_height) {
#ifndef NDEBUG
    ++num_textures;
#endif
  }
#endif

  /**
   * Create a texture with undefined content.
   */
  GLTexture(UPixelScalar _width, UPixelScalar _height);

  GLTexture(GLint internal_format, GLsizei width, GLsizei height,
            GLenum format, GLenum type, const GLvoid *data);

#ifdef ENABLE_SDL
  GLTexture(SDL_Surface *surface) {
    Initialise();
    Load(surface);
  }
#endif

  ~GLTexture() {
    glDeleteTextures(1, &id);

#ifndef NDEBUG
    assert(num_textures > 0);
    --num_textures;
#endif
  }

  /**
   * Returns the standard pixel format of the platform.
   */
  constexpr
  static GLenum GetType() {
    return HaveGLES()
      ? GL_UNSIGNED_SHORT_5_6_5
      : GL_UNSIGNED_BYTE;
  }

  UPixelScalar GetWidth() const {
    return width;
  }

  UPixelScalar GetHeight() const {
    return height;
  }

  gcc_pure
  PixelSize GetSize() const {
    return { width, height };
  }

  /**
   * Returns the physical size of the texture.
   */
  gcc_pure
  PixelSize GetAllocatedSize() const {
#ifdef HAVE_OES_DRAW_TEXTURE
    return GetSize();
#else
    return { allocated_width, allocated_height };
#endif
  }

  /**
   * Change the size of the texture, discarding any previous contents.
   */
  void ResizeDiscard(PixelSize new_size);

protected:
  void Initialise(bool mag_linear=false) {
#ifndef NDEBUG
    ++num_textures;
#endif

    glGenTextures(1, &id);
    Bind();
    Configure(mag_linear);
  }

  static inline void Configure(bool mag_linear=false) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (IsEmbedded()) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    !IsEmbedded() || mag_linear ? GL_LINEAR : GL_NEAREST);
  }

#ifdef ENABLE_SDL
  void Load(SDL_Surface *surface);
#endif

public:
  void Bind() {
    glBindTexture(GL_TEXTURE_2D, id);
  }

  void AttachFramebuffer(GLenum attachment) {
    FBO::FramebufferTexture2D(FBO::FRAMEBUFFER, attachment,
                              GL_TEXTURE_2D, id, 0);
  }

  void Draw(PixelScalar dest_x, PixelScalar dest_y,
            UPixelScalar dest_width, UPixelScalar dest_height,
            PixelScalar src_x, PixelScalar src_y,
            UPixelScalar src_width, UPixelScalar src_height) const;

  void Draw(PixelScalar dest_x, PixelScalar dest_y) const {
    Draw(dest_x, dest_y, width, height,
         0, 0, width, height);
  }

  /**
   * Just like Draw(), but flip the texture vertically.  This is used
   * for textures that were recorded with glCopyTexSubImage2D().
   */
  void DrawFlipped(PixelRect dest, PixelRect src) const;
};

#endif
