/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Util/NonCopyable.hpp"

#ifdef ENABLE_SDL
#include <SDL_video.h>
#include <windef.h>
#else
#include <windows.h>
#endif

#include <assert.h>
#include <tchar.h>

/**
 * An image loaded from storage.
 */
class Bitmap : private NonCopyable {
protected:
#ifdef ENABLE_SDL
  SDL_Surface *surface;
#else
  HBITMAP bitmap;
#endif

public:
#ifdef ENABLE_SDL
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
  ~Bitmap();

public:
  bool defined() const {
#ifdef ENABLE_SDL
    return surface != NULL;
#else
    return bitmap != NULL;
#endif
  }

  bool load(unsigned id);

  /**
   * Load a bitmap and stretch it by the specified zoom factor.
   */
  bool load_stretch(unsigned id, unsigned zoom);

  bool load_file(const TCHAR *path);

  void reset();

  const SIZE get_size() const;

#ifdef ENABLE_SDL
  SDL_Surface* native() const {
    return surface;
  }
#else
  HBITMAP native() const {
    assert(defined());

    return bitmap;
  }
#endif
};

#endif
