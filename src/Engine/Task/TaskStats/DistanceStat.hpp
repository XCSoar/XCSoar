/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifdef DO_PRINT
#include <iostream>
#endif

#include "Util/Filter.hpp"
#include "Util/AvFilter.hpp"
#include "Util/DiffFilter.hpp"

/**
 * Simple distance statistics with derived values (speed, incremental speed)
 * Incremental speeds track the short-term variation of distance with time,
 * whereas the overall speed is defined by the distance divided by a time value.
 */
class DistanceStat
{
  friend class DistanceStatComputer;

protected:
  /** Distance (m) of metric */
  fixed distance;
  /** Speed (m/s) of metric */
  fixed speed;
  /** Incremental speed (m/s) of metric */
  fixed speed_incremental;

public:
  DistanceStat():distance(fixed_zero), speed(fixed_zero) {}

  /**
   * Setter for distance value
   *
   * @param d Distance value (m)
   */
  void set_distance(const fixed d) {
    distance = d;
  }

  /**
   * Accessor for distance value
   *
   * @return Distance value (m)
   */
  fixed get_distance() const {
    return distance;
  }

  /**
   * Accessor for speed
   *
   * @return Speed (m/s)
   */
  fixed get_speed() const {
    return speed;
  }

  /**
   * Accessor for incremental speed (rate of change of
   * distance over dt, low-pass filtered)
   *
   * @return Speed incremental (m/s)
   */
  fixed get_speed_incremental() const {
    return speed_incremental;
  }

  /**
   * Calculate bulk speed (distance/time), abstract base method
   *
   * @param es ElementStat (used for time access)
   */
  void calc_speed(fixed time);

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const DistanceStat& ds);
#endif
};

/**
 * Computer class for DistanceStat.  It holds the incremental and
 * internal values, while DistanceStat has only the results.
 */
class DistanceStatComputer {
protected:
  DistanceStat &data;

private:
  static const unsigned N_AV = 3;

  AvFilter<N_AV> av_dist;
  DiffFilter df;
  Filter v_lpf;
  bool is_positive; // ideally const but then non-copyable

public:
  /**
   * Constructor; initialises all to zero
   *
   */
  DistanceStatComputer(DistanceStat &_data, const bool is_positive=true);

  void calc_speed(fixed time) {
    data.calc_speed(time);
  }

  /**
   * Calculate incremental speed from previous step.
   * Resets incremental speed to speed if dt=0
   *
   * @param dt Time step (s)
   */
  void calc_incremental_speed(const fixed dt);

  fixed get_speed_incremental() const {
    return data.get_speed_incremental();
  }

  fixed get_speed() const {
    return data.get_speed();
  }

private:
  void reset_incremental_speed();
};

#endif
