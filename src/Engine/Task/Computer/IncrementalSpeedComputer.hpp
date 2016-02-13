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

#ifndef XCSOAR_INCREMENTAL_SPEED_COMPUTER_HPP
#define XCSOAR_INCREMENTAL_SPEED_COMPUTER_HPP

#include "Math/Filter.hpp"
#include "Math/AvFilter.hpp"
#include "Math/DiffFilter.hpp"

class DistanceStat;

/**
 * Calculate incremental speed from consecutive distance values.
 */
class IncrementalSpeedComputer {
  static constexpr unsigned N_AV = 3;

  AvFilter<N_AV> av_dist;
  DiffFilter df;
  Filter v_lpf;
  const bool is_positive;

  double last_time;

public:
  /** Constructor; initialises all to zero */
  IncrementalSpeedComputer(const bool is_positive=true);

  /**
   * Calculate incremental speed from previous step.
   * Resets incremental speed to speed if dt=0
   *
   * @param time monotonic time of day in seconds
   */
  void Compute(DistanceStat &data, double time);

  void Reset(DistanceStat &data);
};

#endif
