/* Copyright_License {

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
#ifndef DISTANCE_STAT_HPP
#define DISTANCE_STAT_HPP

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
  double distance;
  /** Speed (m/s) of metric */
  double speed;
  /** Incremental speed (m/s) of metric */
  double speed_incremental;

public:
  void Reset() {
    distance = -1;
    speed = 0;
    speed_incremental = 0;
  }

  bool IsDefined() const {
    return distance >= 0;
  }

  /**
   * Setter for distance value
   *
   * @param d Distance value (m)
   */
  void SetDistance(const double d) {
    distance = d;
  }

  /**
   * Accessor for distance value
   *
   * @return Distance value (m)
   */
  double GetDistance() const {
    assert(IsDefined());

    return distance;
  }

  /**
   * Accessor for speed
   *
   * @return Speed (m/s)
   */
  double GetSpeed() const {
    assert(IsDefined());

    return speed;
  }

  /**
   * Accessor for incremental speed (rate of change of
   * distance over dt, low-pass filtered)
   *
   * @return Speed incremental (m/s)
   */
  double GetSpeedIncremental() const {
    assert(IsDefined());

    return speed_incremental;
  }
};

#endif
