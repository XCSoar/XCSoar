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

#ifndef XCSOAR_SCREEN_DOUBLE_CLICK_HPP
#define XCSOAR_SCREEN_DOUBLE_CLICK_HPP

#include "Time/PeriodClock.hpp"
#include "Screen/Point.hpp"
#include "Asset.hpp"

/**
 * This class helps turning two "mouse down" events into a "double
 * click" event.
 */
class DoubleClick {
  /**
   * The maximum time span between two clicks for a double click [ms].
   */
  static constexpr unsigned INTERVAL_MS = IsKobo()? 750 : 500;

  /**
   * The maximum distance between two clicks.
   */
  static constexpr unsigned MAX_DISTANCE_PX = IsKobo()? 70 : 50;

  PeriodClock clock;

  PixelPoint location;

public:
  /**
   * Reset the object, discard any previous click it may have
   * remembered.
   */
  void Reset() {
    clock.Reset();
  }

  /**
   * Call this in the "mouse up" handler.  It will take care for
   * resetting this object when the mouse/finger has moved too much.
   */
  void Moved(PixelPoint _location) {
    if (clock.IsDefined() &&
        (unsigned)ManhattanDistance(location, _location) > MAX_DISTANCE_PX)
      Reset();
  }

  /**
   * Call this in the "mouse down" handler.
   *
   * @return true if a double click was detected
   */
  bool Check(PixelPoint _location) {
    const bool result = !clock.CheckAlwaysUpdate(INTERVAL_MS) &&
      (unsigned)ManhattanDistance(location, _location) <= MAX_DISTANCE_PX;

    location = _location;
    return result;
  }
};

#endif
