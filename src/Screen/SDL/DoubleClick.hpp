/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_DOUBLE_CLICK_HPP
#define XCSOAR_SCREEN_DOUBLE_CLICK_HPP

#include "PeriodClock.hpp"

/**
 * This class helps turning two "mouse down" events into a "double
 * click" event.
 */
class DoubleClick {
  /**
   * The maximum time span between two clicks for a double click [ms].
   */
  static const unsigned INTERVAL_MS = 500;

  PeriodClock clock;

public:
  /**
   * Reset the object, discard any previous click it may have
   * remembered.
   */
  void Reset() {
    clock.Reset();
  }

  /**
   * Call this in the "mouse down" handler.
   *
   * @return true if a double click was detected
   */
  bool Check() {
    return !clock.CheckAlwaysUpdate(INTERVAL_MS);
  }
};

#endif
