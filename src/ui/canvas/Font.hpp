/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ui/dim/Size.hpp"

#if defined(USE_APPKIT) || defined(USE_UIKIT)
#import <Foundation/Foundation.h>
#endif

#include <cstddef>

#ifdef _WIN32
#include <windef.h>
#endif

#include <tchar.h>

#ifdef USE_FREETYPE
typedef struct FT_FaceRec_ *FT_Face;
#endif

class FontDescription;
class TextUtil;
struct TStringView;

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
#elif defined(USE_APPKIT) || defined(USE_UIKIT)
  NSDictionary *draw_attributes = nil;
#else
#error No font renderer
#endif

  unsigned height, ascent_height, capital_height;

  void CalculateHeights() noexcept;

public:
  Font() noexcept = default;

#if !defined(USE_APPKIT) && !defined(USE_UIKIT)
  ~Font() noexcept { Destroy(); }
#endif

  Font(const Font &other) = delete;
  Font &operator=(const Font &other) = delete;

#ifdef USE_FREETYPE
  /**
   * Perform global font initialisation.
   */
  static void Initialise();
  static void Deinitialise() noexcept;
#endif

public:
  bool IsDefined() const noexcept {
#ifdef USE_FREETYPE
    return face != nullptr;
#elif defined(USE_APPKIT) || defined(USE_UIKIT)
    return nil != draw_attributes;
#elif defined(ANDROID)
    return text_util_object != nullptr;
#else
    return font != nullptr;
#endif
  }

#ifdef USE_FREETYPE
  /**
   * Throws on error.
   */
  void LoadFile(const char *file, unsigned ptsize, bool bold = false,
                bool italic = false);
#endif

  /**
   * Throws on error.
   */
  void Load(const FontDescription &d);

#if defined(USE_APPKIT) || defined(USE_UIKIT)
  void Destroy() noexcept {}
#else
  void Destroy() noexcept;
#endif

  [[gnu::pure]]
  PixelSize TextSize(TStringView text) const noexcept;

  [[gnu::pure]]
  PixelSize TextSize(const TCHAR *text) const noexcept;

#if defined(USE_FREETYPE) || defined(USE_APPKIT) || defined(USE_UIKIT)
  static constexpr std::size_t BufferSize(const PixelSize size) noexcept {
    return std::size_t(size.width) * std::size_t(size.height);
  }

  void Render(TStringView text, const PixelSize size,
              void *buffer) const noexcept;
#elif defined(ANDROID)
  int TextTextureGL(TStringView text, PixelSize &size,
                    PixelSize &allocated_size) const noexcept;
#elif defined(USE_GDI)
  HFONT Native() const noexcept {
    return font;
  }
#endif

  unsigned GetHeight() const noexcept {
    return height;
  }

  unsigned GetAscentHeight() const noexcept {
    return ascent_height;
  }

  unsigned GetCapitalHeight() const noexcept {
    return capital_height;
  }

  unsigned GetLineSpacing() const noexcept {
#ifdef ANDROID
    return line_spacing;
#else
    return height;
#endif
  }
};

#endif
