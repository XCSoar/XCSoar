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

#ifndef XCSOAR_SELF_TIMING_KALMAN_FILTER_1D_HPP
#define XCSOAR_SELF_TIMING_KALMAN_FILTER_1D_HPP

#include "KalmanFilter1d.hpp"

/**
 * Wraps KalmanFilter1d and does its own internal time bookkeeping so
 * that updates need provide only measurement information.
 */
class SelfTimingKalmanFilter1d {
  KalmanFilter1d filter_;

  // Internal time representations are in milliseconds.

  /**
   * Reset if updates are less frequent than this.
   */
  unsigned int max_dt_ms_;

  /**
   * Time of last update.
   */
  unsigned int t_last_update_ms_;

public:
  /**
   * Constructors: like in KalmanFilter1d, but the caller must provide max_dt,
   * which is the maximum time in seconds between updates. If an update takes
   * longer than max_dt seconds, then the filter will reset automatically.
   * The proper way to set max_dt is to consider how many seconds you can go
   * without risking numerical instability: a good rule of thumb here would
   * be to keep var_x_accel*max_dt^4 in the realm of stability.
   *
   * Or you can just punt and set it to, like, a minute.
   */
  SelfTimingKalmanFilter1d(const fixed max_dt, const fixed var_x_accel);
  SelfTimingKalmanFilter1d(const fixed max_dt);

  // Get and set the maximum update interval as desired. See note above
  // constructors for details on update intervals.
  void SetMaxDt(const fixed max_dt);
  fixed GetMaxDt() const;

  /**
   * Updates state given a direct sensor measurement of the absolute
   * quantity x and the variance of that measurement. Appropriate
   * timimg information will be supplied by this object, with the
   * filter resetting automatically for updates separated by large
   * time intervals as described above.
   */
  void Update(const fixed z_abs, const fixed var_z_abs);

  // Remaining methods are identical to their counterparts in KalmanFilter1d.

  void Reset() {
    filter_.Reset();
  }

  void Reset(const fixed x_abs_value) {
    filter_.Reset(x_abs_value);
  }

  void Reset(const fixed x_abs_value, const fixed x_vel_value) {
    filter_.Reset(x_abs_value, x_vel_value);
  }

  void SetAccelerationVariance(const fixed var_x_accel) {
    filter_.SetAccelerationVariance(var_x_accel);
  }

  fixed GetXAbs() const { return filter_.GetXAbs(); }
  fixed GetXVel() const { return filter_.GetXVel(); }
  fixed GetCovAbsAbs() const { return filter_.GetCovAbsAbs(); }
  fixed GetCovAbsVel() const { return filter_.GetCovAbsVel(); }
  fixed GetCovVelVel() const { return filter_.GetCovVelVel(); }
};

#endif
