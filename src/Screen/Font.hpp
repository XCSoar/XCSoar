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

#ifndef XCSOAR_SCREEN_FONT_HPP
#define XCSOAR_SCREEN_FONT_HPP

#include "Util/NonCopyable.hpp"

#ifdef ENABLE_SDL
#include <SDL_ttf.h>
#endif

#include <windows.h>

/**
 * A font loaded from storage.  It is used by #Canvas to draw text.
 */
class Font : private NonCopyable {
protected:
  #ifdef ENABLE_SDL
  TTF_Font *font;
  #else
  HFONT font;
  #endif

  unsigned height, ascent_height, capital_height;

  void calculate_heights();

public:
  Font():font(NULL) {}
  ~Font() { reset(); }

public:
  bool
  defined() const
  {
    return font != NULL;
  }

  #ifdef ENABLE_SDL
  bool set(const char *file, int ptsize, bool bold = false,
           bool italic = false);
#ifdef _UNICODE
  bool set(const TCHAR *facename, int height, bool bold = false,
           bool italic = false);
#endif
  #else
  bool set(const TCHAR* facename, int height, bool bold = false,
           bool italic = false);
  #endif
  bool set(const LOGFONT &log_font);

  void reset();

  #ifdef ENABLE_SDL
  TTF_Font*
  native() const {
    return font;
  }
  #else
  HFONT
  native() const {
    return font;
  }
  #endif

  unsigned get_height() const {
    return height;
  }

  unsigned get_ascent_height() const {
    return ascent_height;
  }

  unsigned get_capital_height() const {
    return capital_height;
  }
};

#endif
