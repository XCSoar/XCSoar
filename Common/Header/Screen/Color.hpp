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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * This class represents a color in the RGB color space.  This is used
 * for compile-time constant colors, or for colors loaded from the
 * configuration.
 */
struct Color {
  COLORREF value;

  Color():value(RGB(0, 0, 0)) {}
  Color(COLORREF c):value(c) {}
  Color(int r, int g, int b):value(RGB(r, g, b)) {}

  unsigned char red() const {
    return GetRValue(value);
  }

  unsigned char green() const {
    return GetGValue(value);
  }

  unsigned char blue() const {
    return GetBValue(value);
  }

  Color &operator =(COLORREF c) {
    value = c;
    return *this;
  }

  operator COLORREF() const {
    return value;
  }

  /**
   * Returns the highlighted version of this color.
   */
  Color highlight() const {
    return Color((value + 0x00ffffff * 3) / 4);
  }
};

static inline bool
operator ==(const Color a, const Color b)
{
  return a.value == b.value;
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
  COLORREF value;

  HWColor():value(0) {}
  HWColor(COLORREF c):value(c) {}

  operator COLORREF() const {
    return value;
  }
};

#endif
