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

#ifndef XCSOAR_SCREEN_FONT_HPP
#define XCSOAR_SCREEN_FONT_HPP

#include "Util/NonCopyable.hpp"
#include "Compiler.h"

#ifdef ANDROID
#include "Util/tstring.hpp"
#include "Screen/Color.hpp"
#else  // !ANDROID
#ifdef ENABLE_SDL
#include <SDL_ttf.h>
#endif
#endif // !ANDROID

#include <windows.h>
#include <tchar.h>

class TextUtil;

/**
 * A font loaded from storage.  It is used by #Canvas to draw text.
 */
class Font : private NonCopyable {
protected:
  #ifdef ANDROID
  TextUtil *textUtilObject;

  unsigned line_spacing, style;
  tstring facename;
  #else // !ANDROID
  #ifdef ENABLE_SDL
  TTF_Font *font;
  #else
  HFONT font;
  #endif
  #endif

  unsigned height, ascent_height, capital_height;

  void calculate_heights();
  #ifndef ANDROID
  #ifdef ENABLE_SDL
  bool _set(const char *file, int ptsize, bool bold = false,
            bool italic = false);
  #endif
  #endif

public:
  #ifdef ANDROID
  Font():textUtilObject(NULL) {}
  #else
  Font():font(NULL) {}
  #endif
  ~Font() { reset(); }

  /**
   * Perform global font initialisation.
   */
  static void Initialise();

public:
  bool
  defined() const {
    #ifdef ANDROID
    return textUtilObject != NULL;
    #else
    return font != NULL;
    #endif
  }

  bool set(const TCHAR *facename, int height, bool bold = false,
           bool italic = false);
  bool set(const LOGFONT &log_font);
  void reset();

  #ifdef ANDROID
  gcc_pure
  PixelSize TextSize(const TCHAR *text) const;

  int text_texture_gl(const TCHAR *text, PixelSize &size,
                      const Color &fg, const Color &bg) const;
  #else // !ANDROID
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
  #endif // !ANDROID

  unsigned get_height() const {
    return height;
  }
  unsigned get_ascent_height() const {
    return ascent_height;
  }
  unsigned get_capital_height() const {
    return capital_height;
  }
  #ifdef ANDROID
  unsigned get_line_spacing() const {
    return line_spacing;
  }
  unsigned get_style() const {
    return style;
  }
  const TCHAR *get_facename() const {
    return facename.c_str();
  }
  #endif
};

#endif
