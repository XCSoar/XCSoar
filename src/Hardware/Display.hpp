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

#ifndef XCSOAR_HARDWARE_DISPLAY_H
#define XCSOAR_HARDWARE_DISPLAY_H

#include "Compiler.h"

namespace Display {
#if defined(_WIN32_WCE) && !defined(GNAV)
#define HAVE_HARDWARE_BLANK
  bool BlankSupported();
  bool Blank(bool blank);
#else
  static inline bool BlankSupported() {
    return false;
  }

  static inline bool Blank(bool blank) {
    return false;
  }
#endif

#ifdef _WIN32_WCE
  bool SetBacklight();
#else
  static inline bool SetBacklight() {
    return false;
  }
#endif

  enum orientation {
    ORIENTATION_DEFAULT,
    ORIENTATION_PORTRAIT,
    ORIENTATION_LANDSCAPE,
  };

  gcc_const
  bool RotateSupported();

  /**
   * Change the orientation of the screen.
   */
  bool
  Rotate(enum orientation orientation);

  /**
   * Restores the display rotation setting.
   */
  bool
  RotateRestore();

  /**
   * Returns the number of pixels per logical inch along the screen width
   * @return Number of pixels per logical inch along the screen width
   */
  int GetXDPI();
  /**
   * Returns the number of pixels per logical inch along the screen height
   * @return Number of pixels per logical inch along the screen height
   */
  int GetYDPI();
}

#endif
