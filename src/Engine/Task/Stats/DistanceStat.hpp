/* Copyright_License {

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
#ifndef DISTANCE_STAT_HPP
#define DISTANCE_STAT_HPP

#include "Math/fixed.hpp"

#include <assert.h>

/**
 * Simple distance statistics with derived values (speed, incremental speed)
 * Incremental speeds track the short-term variation of distance with time,
 * whereas the overall speed is defined by the distance divided by a time value.
 */
class DistanceStat
{
  friend class DistanceStatComputer;
  friend class IncrementalSpeedComputer;

protected:
  /** Distance (m) of metric */
  fixed distance;
  /** Speed (m/s) of metric */
  fixed speed;
  /** Incremental speed (m/s) of metric */
  fixed speed_incremental;

public:
  void Reset() {
    distance = fixed(-1);
    speed = fixed(0);
    speed_incremental = fixed(0);
  }

  bool IsDefined() const {
    return !negative(distance);
  }

  /**
   * Setter for distance value
   *
   * @param d Distance value (m)
   */
  void SetDistance(const fixed d) {
    distance = d;
  }

  /**
   * Accessor for distance value
   *
   * @return Distance value (m)
   */
  fixed GetDistance() const {
    assert(IsDefined());

    return distance;
  }

  /**
   * Accessor for speed
   *
   * @return Speed (m/s)
   */
  fixed GetSpeed() const {
    assert(IsDefined());

    return speed;
  }

  /**
   * Accessor for incremental speed (rate of change of
   * distance over dt, low-pass filtered)
   *
   * @return Speed incremental (m/s)
   */
  fixed GetSpeedIncremental() const {
    assert(IsDefined());

    return speed_incremental;
  }
};

#endif
