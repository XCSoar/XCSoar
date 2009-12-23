/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifdef ENABLE_SDL
#include <SDL/SDL_video.h>
#include <windef.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <tchar.h>

/**
 * An image loaded from storage.
 */
class Bitmap {
protected:
#ifdef ENABLE_SDL
  SDL_Surface *surface;
#else
  HBITMAP bitmap;
#endif

public:
#ifdef ENABLE_SDL
  Bitmap():surface(NULL) {}
  explicit Bitmap(const TCHAR *name):surface(NULL) {
    load(name);
  }
  explicit Bitmap(WORD id):surface(NULL) {
    load(id);
  }
#else
  Bitmap():bitmap(NULL) {}
  explicit Bitmap(const TCHAR *name):bitmap(NULL) {
    load(name);
  }
  explicit Bitmap(WORD id):bitmap(NULL) {
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

  void load(const TCHAR *name);

#ifdef ENABLE_SDL
  void load(WORD id);
#else
  void load(WORD id) {
    load(MAKEINTRESOURCE(id));
  }
#endif

  void *create(unsigned width, unsigned height);

  void reset();

  const SIZE get_size() const;

#ifdef ENABLE_SDL
  SDL_Surface* native() const {
    return surface;
  }
#else
  HBITMAP native() const {
    return bitmap;
  }
#endif
};

#endif
