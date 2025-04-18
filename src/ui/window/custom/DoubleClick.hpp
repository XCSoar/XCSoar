// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/PeriodClock.hpp"
#include "ui/dim/Point.hpp"
#include "Asset.hpp"

/**
 * This class helps turning two "mouse down" events into a "double
 * click" event.
 */
class DoubleClick {
  /**
   * The maximum time span between two clicks for a double click [ms].
   */
  static constexpr auto INTERVAL = std::chrono::milliseconds(IsKobo() ? 750 : 500);

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
  void Reset() noexcept {
    clock.Reset();
  }

  /**
   * Call this in the "mouse up" handler.  It will take care for
   * resetting this object when the mouse/finger has moved too much.
   */
  void Moved(PixelPoint _location) noexcept {
    if (clock.IsDefined() &&
        (unsigned)ManhattanDistance(location, _location) > MAX_DISTANCE_PX)
      Reset();
  }

  /**
   * Call this in the "mouse down" handler.
   *
   * @return true if a double click was detected
   */
  bool Check(PixelPoint _location) noexcept {
    const bool result = !clock.CheckAlwaysUpdate(INTERVAL) &&
      (unsigned)ManhattanDistance(location, _location) <= MAX_DISTANCE_PX;

    location = _location;
    return result;
  }
};
