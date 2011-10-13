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

#ifndef XCSOAR_SCREEN_BITMAP_HPP
#define XCSOAR_SCREEN_BITMAP_HPP

#include "Screen/Point.hpp"
#include "Util/NonCopyable.hpp"

#ifdef ENABLE_OPENGL
#include "Util/tstring.hpp"
#include "Screen/OpenGL/Surface.hpp"
#endif

#ifdef ANDROID
#elif defined(ENABLE_SDL)
#include <SDL_video.h>
#else
#include <windows.h>
#endif

#include <assert.h>
#include <tchar.h>

#ifdef ENABLE_OPENGL
class GLTexture;
#endif

/**
 * An image loaded from storage.
 */
class Bitmap : private NonCopyable
#ifdef ENABLE_OPENGL
             , private GLSurfaceListener
#endif
{
protected:
#ifdef ENABLE_OPENGL
  /** resource id */
  unsigned id;
  /** filename for external images (id=0) */
  tstring pathName;
  GLTexture *texture;
  UPixelScalar width, height;
#elif defined(ENABLE_SDL)
  SDL_Surface *surface;
#else
  HBITMAP bitmap;
#endif

public:
#ifdef ENABLE_OPENGL
  Bitmap():id(0), texture(NULL) {}
  explicit Bitmap(unsigned id):texture(NULL) {
    load(id);
  }
#elif defined(ENABLE_SDL)
  Bitmap():surface(NULL) {}
  explicit Bitmap(unsigned id):surface(NULL) {
    load(id);
  }
#else
  Bitmap():bitmap(NULL) {}
  explicit Bitmap(unsigned id):bitmap(NULL) {
    load(id);
  }
#endif

  ~Bitmap() {
    reset();
  }

public:
  bool defined() const {
#ifdef ENABLE_OPENGL
    return texture != NULL;
#elif defined(ENABLE_SDL)
    return surface != NULL;
#else
    return bitmap != NULL;
#endif
  }

#ifdef ENABLE_OPENGL
  UPixelScalar get_width() const {
    return width;
  }

  UPixelScalar get_height() const {
    return height;
  }
#endif

#ifndef ANDROID
#ifdef ENABLE_SDL
  bool load(SDL_Surface *_surface);
#endif
#endif

  bool load(unsigned id);

  /**
   * Load a bitmap and stretch it by the specified zoom factor.
   */
  bool load_stretch(unsigned id, unsigned zoom);

  bool load_file(const TCHAR *path);

  void reset();

  const PixelSize get_size() const;

#ifdef ENABLE_OPENGL
  GLTexture *native() const {
    return texture;
  }
#elif defined(ENABLE_SDL)
  SDL_Surface* native() const {
    return surface;
  }
#else
  HBITMAP native() const {
    assert(defined());

    return bitmap;
  }
#endif

#ifdef ENABLE_OPENGL
private:
  /**
   * Load the texture again after the OpenGL surface has been
   * recreated.
   */
  bool Reload();

  /* from GLSurfaceListener */
  virtual void surface_created();
  virtual void surface_destroyed();
#endif
};

#endif
