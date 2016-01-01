/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Screen/Point.hpp"
#include "Compiler.h"

#ifdef USE_FREETYPE
typedef struct FT_FaceRec_ *FT_Face;
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <tchar.h>

class FontDescription;
class TextUtil;

/**
 * A font loaded from storage.  It is used by #Canvas to draw text.
 */
class Font {
protected:
#ifdef USE_FREETYPE
  FT_Face face = nullptr;
#elif defined(ANDROID)
  TextUtil *text_util_object = nullptr;

  unsigned line_spacing;
#elif defined(USE_GDI)
  HFONT font = nullptr;
#else
#error No font renderer
#endif

  unsigned height, ascent_height, capital_height;

  void CalculateHeights();

public:
  Font() = default;

  ~Font() { Destroy(); }

  Font(const Font &other) = delete;
  Font &operator=(const Font &other) = delete;

#ifdef USE_FREETYPE
  /**
   * Perform global font initialisation.
   */
  static void Initialise();
  static void Deinitialise();
#endif

public:
  bool
  IsDefined() const {
#ifdef USE_FREETYPE
    return face != nullptr;
#elif defined(ANDROID)
    return text_util_object != nullptr;
    #else
    return font != nullptr;
    #endif
  }

#ifdef USE_FREETYPE
  bool LoadFile(const char *file, unsigned ptsize, bool bold = false,
                bool italic = false);
#endif

  bool Load(const FontDescription &d);
  void Destroy();

  gcc_pure
  PixelSize TextSize(const TCHAR *text) const;

#ifdef USE_FREETYPE
  gcc_const
  static size_t BufferSize(const PixelSize size) {
    return size.cx * size.cy;
  }

  void Render(const TCHAR *text, const PixelSize size, void *buffer) const;
#elif defined(ANDROID)
  int TextTextureGL(const TCHAR *text, PixelSize &size,
                    PixelSize &allocated_size) const;
#elif defined(USE_GDI)
  HFONT Native() const {
    return font;
  }
#endif

  unsigned GetHeight() const {
    return height;
  }
  unsigned GetAscentHeight() const {
    return ascent_height;
  }
  unsigned GetCapitalHeight() const {
    return capital_height;
  }

#ifdef USE_FREETYPE
  unsigned GetLineSpacing() const {
    return height;
  }
#elif defined(ANDROID)
  unsigned GetLineSpacing() const {
    return line_spacing;
  }
#endif
};

#endif
