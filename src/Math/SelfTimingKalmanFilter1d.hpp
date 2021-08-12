/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include <chrono>
#include <cstdint>

/**
 * Wraps KalmanFilter1d and does its own internal time bookkeeping so
 * that updates need provide only measurement information.
 */
class SelfTimingKalmanFilter1d {
public:
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using TimePoint = Clock::time_point;

private:
  KalmanFilter1d filter_;

  // Internal time representations are in milliseconds.

  /**
   * Reset if updates are less frequent than this.
   */
  Duration max_dt;

  /**
   * Time of last update.
   */
  TimePoint last_update_time;

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
  SelfTimingKalmanFilter1d(Duration _max_dt,
                           double var_x_accel=1) noexcept
    :filter_(var_x_accel), max_dt(_max_dt) {}

  /**
   * Updates state given a direct sensor measurement of the absolute
   * quantity x and the variance of that measurement. Appropriate
   * timimg information will be supplied by this object, with the
   * filter resetting automatically for updates separated by large
   * time intervals as described above.
   */
  void Update(double z_abs, double var_z_abs) noexcept;

  // Remaining methods are identical to their counterparts in KalmanFilter1d.

  void Reset(const double x_abs_value=0, const double x_vel_value=0) noexcept {
    filter_.Reset(x_abs_value, x_vel_value);
  }

  void SetAccelerationVariance(const double var_x_accel) noexcept {
    filter_.SetAccelerationVariance(var_x_accel);
  }

  double GetXAbs() const noexcept { return filter_.GetXAbs(); }
  double GetXVel() const noexcept { return filter_.GetXVel(); }
  double GetCovAbsAbs() const noexcept { return filter_.GetCovAbsAbs(); }
  double GetCovAbsVel() const noexcept { return filter_.GetCovAbsVel(); }
  double GetCovVelVel() const noexcept { return filter_.GetCovVelVel(); }
};

#endif
