/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
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

#ifndef XCSOAR_SCREEN_OPENGL_TEXTURE_HPP
#define XCSOAR_SCREEN_OPENGL_TEXTURE_HPP

#include "Asset.hpp"

#include <assert.h>

#ifdef ANDROID
#include <GLES/gl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
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
  unsigned width, height;

#ifndef ANDROID
  /**
   * The real dimensions of the texture.  This may differ when
   * ARB_texture_non_power_of_two is not available.
   */
  GLsizei allocated_width, allocated_height;
#endif

public:
#ifdef ANDROID
  GLTexture(GLuint _id, unsigned _width, unsigned _height)
    :id(_id), width(_width), height(_height) {
#ifndef NDEBUG
    ++num_textures;
#endif
  }
#endif

  /**
   * Create a texture with undefined content.
   */
  GLTexture(unsigned _width, unsigned _height);

#ifndef ANDROID
  GLTexture(SDL_Surface *surface) {
    init();
    load(surface);
  }
#endif

  ~GLTexture() {
    glDeleteTextures(1, &id);

#ifndef NDEBUG
    assert(num_textures > 0);
    --num_textures;
#endif
  }

  unsigned get_width() const {
    return width;
  }

  unsigned get_height() const {
    return height;
  }

protected:
  void init(bool mag_linear=false) {
#ifndef NDEBUG
    ++num_textures;
#endif

    glGenTextures(1, &id);
    bind();
    configure(mag_linear);
  }

  static inline void configure(bool mag_linear=false) {
    if (is_embedded()) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    !is_embedded() || mag_linear ? GL_LINEAR : GL_NEAREST);
  }

#ifndef ANDROID
  void load(SDL_Surface *surface);
#endif

public:
  void bind() {
    glBindTexture(GL_TEXTURE_2D, id);
  }

  void draw(int dest_x, int dest_y,
            unsigned dest_width, unsigned dest_height,
            int src_x, int src_y,
            unsigned src_width, unsigned src_height) const;

  void draw(int dest_x, int dest_y) const {
    draw(dest_x, dest_y, width, height,
         0, 0, width, height);
  }
};

#endif
