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

#ifndef XCSOAR_DISTANCE_STAT_COMPUTER_HPP
#define XCSOAR_DISTANCE_STAT_COMPUTER_HPP

#include "IncrementalSpeedComputer.hpp"

class DistanceStat;

/**
 * Computer class for DistanceStat.  It holds the incremental and
 * internal values, while DistanceStat has only the results.
 */
class DistanceStatComputer {
  IncrementalSpeedComputer incremental_speed;

public:
  /** Constructor; initialises all to zero */
  DistanceStatComputer(const bool is_positive=true)
    :incremental_speed(is_positive) {}

  /**
   * Calculate bulk speed (distance/time), abstract base method
   *
   * @param es ElementStat (used for time access)
   */
  void CalcSpeed(DistanceStat &data, double time);

  /**
   * Calculate incremental speed from previous step.
   * Resets incremental speed to speed if dt=0
   *
   * @param time monotonic time of day in seconds
   */
  void CalcIncrementalSpeed(DistanceStat &data, double time) {
    incremental_speed.Compute(data, time);
  }

  void ResetIncrementalSpeed(DistanceStat &data) {
    incremental_speed.Reset(data);
  }
};

#endif
