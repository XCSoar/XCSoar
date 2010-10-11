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

#ifndef XCSOAR_SCREEN_FONT_HPP
#define XCSOAR_SCREEN_FONT_HPP

#include "Util/NonCopyable.hpp"

#ifdef ENABLE_SDL
  #include <SDL/SDL_ttf.h>
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
