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

#ifndef XCSOAR_SCREEN_BRUSH_HPP
#define XCSOAR_SCREEN_BRUSH_HPP

#include "Screen/Color.hpp"

class Bitmap;

class Brush {
protected:
  #ifdef ENABLE_SDL
  bool hollow;
  Color color;
  #else
  HBRUSH brush;
  #endif

public:
  #ifdef ENABLE_SDL
  Brush():hollow(true) {}
  explicit Brush(const Color _color):hollow(false), color(_color)  {}
  #else
  Brush():brush(NULL) {}
  explicit Brush(const Color c):brush(NULL) {
    set(c);
  }
  #endif
  ~Brush() { reset(); }

public:
  void set(const Color c);
  void set(const Bitmap &bitmap);
  void reset();

  bool
  defined() const
  {
    #ifdef ENABLE_SDL
    return !hollow;
    #else
    return brush != NULL;
    #endif
  }

  #ifdef ENABLE_SDL
  bool is_hollow() const { return hollow; }
  const Color get_color() const { return color; }
  #else
  HBRUSH native() const {
    return brush;
  }
  #endif
};

#endif
