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

#ifndef XCSOAR_SCREEN_PEN_HPP
#define XCSOAR_SCREEN_PEN_HPP

#include "Screen/Color.hpp"

class Pen {
public:
  enum style {
    SOLID = PS_SOLID,
    DASH = PS_DASH,
    BLANK = PS_NULL
  };

protected:
  HPEN pen;

public:
  Pen():pen(NULL) {}
  Pen(enum style style, unsigned width, const Color c):pen(NULL) {
    set(style, width, c);
  }
  Pen(unsigned width, Color c):pen(NULL) {
    set(width, c);
  }
  ~Pen() { reset(); }

public:
  void set(enum style style, unsigned width, const Color c);
  void set(unsigned width, const Color c);
  void reset();

  bool defined() const {
    return pen != NULL;
  }

  HPEN native() const {
    return pen;
  }
};

#endif
