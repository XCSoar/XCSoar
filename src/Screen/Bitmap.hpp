/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifdef ENABLE_OPENGL
#include "Util/tstring.hpp"
#include "Screen/OpenGL/Surface.hpp"
#endif

#ifdef USE_GDI
#include <windows.h>
#endif

#include <assert.h>
#include <tchar.h>
#include <stdint.h>

#ifdef ENABLE_OPENGL
class GLTexture;
#endif

#ifdef ENABLE_SDL
struct SDL_Surface;
#endif

/**
 * An image loaded from storage.
 */
class Bitmap
#ifdef ENABLE_OPENGL
             : private GLSurfaceListener
#endif
{
public:
  enum class Type {
    /**
     * A standard bitmap that will be blitted to the screen.  After
     * loading, it will be converted to the screen's pixel format.
     */
    STANDARD,

    /**
     * A monochrome bitmap (1 bit per pixel).
     */
    MONO,
  };

protected:
#ifdef ENABLE_OPENGL
  /** resource id */
  unsigned id;
  /** filename for external images (id=0) */
  tstring pathName;
  GLTexture *texture;
  PixelSize size;
#elif defined(ENABLE_SDL)
  SDL_Surface *surface;
#else
  HBITMAP bitmap;
#endif

public:
#ifdef ENABLE_OPENGL
  Bitmap():id(0), texture(NULL) {}
  explicit Bitmap(unsigned id):texture(NULL) {
    Load(id);
  }
#elif defined(ENABLE_SDL)
  Bitmap():surface(NULL) {}
  explicit Bitmap(unsigned id):surface(NULL) {
    Load(id);
  }
#else
  Bitmap():bitmap(NULL) {}
  explicit Bitmap(unsigned id):bitmap(NULL) {
    Load(id);
  }
#endif

  ~Bitmap() {
    Reset();
  }

  Bitmap(const Bitmap &other) = delete;
  Bitmap &operator=(const Bitmap &other) = delete;

public:
  bool IsDefined() const {
#ifdef ENABLE_OPENGL
    return texture != NULL;
#elif defined(ENABLE_SDL)
    return surface != NULL;
#else
    return bitmap != NULL;
#endif
  }

#ifdef ENABLE_OPENGL
  UPixelScalar GetWidth() const {
    return size.cx;
  }

  UPixelScalar GetHeight() const {
    return size.cy;
  }
#endif

#ifdef ENABLE_SDL
  bool Load(SDL_Surface *_surface, Type type=Type::STANDARD);
#endif

  bool Load(unsigned id, Type type=Type::STANDARD);

  /**
   * Load a bitmap and stretch it by the specified zoom factor.
   */
  bool LoadStretch(unsigned id, unsigned zoom);

  bool LoadFile(const TCHAR *path);

  void Reset();

  const PixelSize GetSize() const;

#ifdef ENABLE_OPENGL
  GLTexture *GetNative() const {
    return texture;
  }
#elif defined(ENABLE_SDL)
  SDL_Surface* GetNative() const {
    return surface;
  }
#else
  HBITMAP GetNative() const {
    assert(IsDefined());

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
  virtual void SurfaceCreated() override;
  virtual void SurfaceDestroyed() override;
#endif
};

#endif
