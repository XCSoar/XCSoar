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

#ifndef XCSOAR_SCREEN_PEN_HPP
#define XCSOAR_SCREEN_PEN_HPP

#include "Screen/Color.hpp"

/**
 * A pen draws lines and borders.
 */
class Pen {
public:
#ifdef ENABLE_SDL
  enum style {
    SOLID,
    DASH,
    BLANK
  };
#else
  enum style {
    SOLID = PS_SOLID,
    DASH = PS_DASH,
    BLANK = PS_NULL
  };
#endif

protected:
#ifdef ENABLE_SDL
  unsigned width;
  Color color;
#else
  HPEN pen;
#endif

public:
#ifdef ENABLE_SDL
  Pen():width(0) {}
  Pen(enum style style, unsigned _width, const Color _color)
    :width(_width), color(_color) {} // XXX style
  Pen(unsigned _width, const Color _color)
    :width(_width), color(_color) {}
#else /* !ENABLE_SDL */
  Pen():pen(NULL) {}
  Pen(enum style style, unsigned width, const Color c):pen(NULL) {
    set(style, width, c);
  }
  Pen(unsigned width, Color c):pen(NULL) {
    set(width, c);
  }
#endif /* !ENABLE_SDL */

  ~Pen() { reset(); }

public:
  void set(enum style style, unsigned width, const Color c);
  void set(unsigned width, const Color c);
  void reset();

  bool defined() const {
#ifdef ENABLE_SDL
    return width > 0;
#else
    return pen != NULL;
#endif
  }

#ifdef ENABLE_SDL
  unsigned get_width() const { return width; }
  const Color get_color() const { return color; }
#else
  HPEN native() const {
    return pen;
  }
#endif
};

#endif
