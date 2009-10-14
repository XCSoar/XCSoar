/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#ifndef XCSOAR_SCREEN_COLOR_HPP
#define XCSOAR_SCREEN_COLOR_HPP

#ifdef ENABLE_SDL
#include <SDL/SDL_video.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/**
 * This class represents a color in the RGB color space.  This is used
 * for compile-time constant colors, or for colors loaded from the
 * configuration.
 */
struct Color {
#ifdef ENABLE_SDL
  SDL_Color value;
#else
  COLORREF value;
#endif

#ifdef ENABLE_SDL
  Color() {
    value.r = 0; value.g = 0; value.b = 0;
    value.unused = SDL_ALPHA_OPAQUE;
  }
  Color(int r, int g, int b) {
    value.r = r;
    value.g = g;
    value.b = b;
    value.unused = SDL_ALPHA_OPAQUE; // alpha for SDL_gfx, see gfx_color()
  }
#else
  Color():value(RGB(0, 0, 0)) {}
  Color(COLORREF c):value(c) {}
  Color(int r, int g, int b):value(RGB(r, g, b)) {}
#endif

  unsigned char red() const {
#ifdef ENABLE_SDL
    return value.r;
#else
    return GetRValue(value);
#endif
  }

  unsigned char green() const {
#ifdef ENABLE_SDL
    return value.g;
#else
    return GetGValue(value);
#endif
  }

  unsigned char blue() const {
#ifdef ENABLE_SDL
    return value.b;
#else
    return GetBValue(value);
#endif
  }

#ifdef ENABLE_SDL
  operator const SDL_Color() const {
    return value;
  }

  Uint32 gfx_color() const {
    return *(const Uint32*)&value;
  }
#else
  Color &operator =(COLORREF c) {
    value = c;
    return *this;
  }

  operator COLORREF() const {
    return value;
  }
#endif

  /**
   * Returns the highlighted version of this color.
   */
  Color highlight() const {
#ifdef ENABLE_SDL
    return Color((value.r + 0xff * 3) / 4,
                 (value.g + 0xff * 3) / 4,
                 (value.b + 0xff * 3) / 4);
#else
    return Color((value + 0x00ffffff * 3) / 4);
#endif
  }
};

static inline bool
operator ==(const Color a, const Color b)
{
#ifdef ENABLE_SDL
  return a.value.r == b.value.r && a.value.g == b.value.g &&
    a.value.b == b.value.b;
#else
  return a.value == b.value;
#endif
}

static inline bool
operator !=(const Color a, const Color b)
{
  return !(a == b);
}

/**
 * A hardware color on a specific #Canvas.  A #Canvas maps a #Color
 * object into #HWColor.  Depending on the platform, #Color and
 * #HWColor may be different, e.g. if the #Canvas can not display 24
 * bit RGB colors.
 */
struct HWColor {
#ifdef ENABLE_SDL
  Uint32 value;
#else
  COLORREF value;
#endif

  HWColor():value(0) {}
#ifdef ENABLE_SDL
  HWColor(Uint32 c):value(c) {}
#else
  HWColor(COLORREF c):value(c) {}
#endif

#ifdef ENABLE_SDL
  operator Uint32() const {
    return value;
  }
#else
  operator COLORREF() const {
    return value;
  }
#endif
};

#endif
