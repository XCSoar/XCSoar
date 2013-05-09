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

#ifndef XCSOAR_DISTANCE_STAT_COMPUTER_HPP
#define XCSOAR_DISTANCE_STAT_COMPUTER_HPP

#include "Math/fixed.hpp"
#include "Math/Filter.hpp"
#include "Math/AvFilter.hpp"
#include "Math/DiffFilter.hpp"

class DistanceStat;

/**
 * Computer class for DistanceStat.  It holds the incremental and
 * internal values, while DistanceStat has only the results.
 */
class DistanceStatComputer {
private:
  static const unsigned N_AV = 3;

  AvFilter<N_AV> av_dist;
  DiffFilter df;
  Filter v_lpf;
  bool is_positive; // ideally const but then non-copyable

public:
  /** Constructor; initialises all to zero */
  DistanceStatComputer(const bool is_positive=true);

  /**
   * Calculate bulk speed (distance/time), abstract base method
   *
   * @param es ElementStat (used for time access)
   */
  void CalcSpeed(DistanceStat &data, fixed time);

  /**
   * Calculate incremental speed from previous step.
   * Resets incremental speed to speed if dt=0
   *
   * @param dt Time step (s)
   */
  void CalcIncrementalSpeed(DistanceStat &data, const fixed dt);

private:
  void ResetIncrementalSpeed(DistanceStat &data);
};

#endif
