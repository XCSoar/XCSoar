// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
