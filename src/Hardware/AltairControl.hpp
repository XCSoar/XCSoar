/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_HARDWARE_ALTAIR_HPP
#define XCSOAR_HARDWARE_ALTAIR_HPP
#ifdef GNAV

#include <windows.h>

/**
 * A driver for the Altair specific device "TRA1:".
 */
class AltairControl {
  HANDLE handle;

public:
  AltairControl();
  ~AltairControl();

  bool ShortBeep();

  /**
   * Reads the backlight value; range is 0..100 or -100 for auto
   * backlight.
   */
  bool GetBacklight(int &value_r);

  /**
   * Sets a new backlight value; range is 0..100, or -100 for auto
   * backlight.
   */
  bool SetBacklight(int value);
};

#endif
#endif
