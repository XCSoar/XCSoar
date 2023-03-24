// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * A Kalman filter that estimates a one-dimensional quantity "x" and
 * its rate of change. Observations are of the one-dimensional
 * quantity itself. Rate of change is assumed to be perturbed by
 * Gaussian noise with a configurable variance. There are assumed to
 * be no control inputs.
 *
 * The notation, arithmetic, and the model itself borrow from the "truck"
 * example on this version of the Wikipedia page for Kalman filters:
 *   http://en.wikipedia.org/w/index.php?title=Kalman_filter&oldid=484054295
 * This implementation is devised from public domain code available here:
 *   https://code.google.com/p/pressure-altimeter/
 */
class KalmanFilter1d {
  // The state we are tracking, namely:
  double x_abs_;  // The absolute quantity x.
  double x_vel_;  // The rate of change of x, in x units per second squared.

  // Covariance matrix for the state.
  double p_abs_abs_;
  double p_abs_vel_;
  double p_vel_vel_;

  // The variance of the acceleration noise input to the system model, in units
  // per second squared.
  double var_x_accel_;

 public:
  // Constructors: the first allows you to supply the variance of the
  // acceleration noise input to the system model in x units per second squared;
  // the second constructor assumes a variance of 1.0.
  explicit KalmanFilter1d(double var_x_accel=1) noexcept;

  // The following three methods reset the filter. All of them assign a huge
  // variance to the tracked absolute quantity and a var_x_accel_ variance to
  // its derivative, so the very next measurement will essentially be copied
  // directly into the filter. Still, we provide methods that allow you to
  // specify initial settings for the filter's tracked state.
  //
  // NOTE: "x_abs_value" is meant to connote the value of the absolute quantity
  // x, not the absolute value of x.
  void Reset(double x_abs_value=0, double x_vel_value=0) noexcept;

  /**
   * Sets the variance of the acceleration noise input to the system model in
   * x units per second squared.
   */
  void SetAccelerationVariance(double var_x_accel) noexcept {
    var_x_accel_ = var_x_accel;
  }

  /**
   * Updates state given a direct sensor measurement of the absolute
   * quantity x, the variance of that measurement, and the interval
   * since the last measurement in seconds. This interval must be
   * greater than 0; for the first measurement after a Reset(), it's
   * safe to use 1.0.
   */
  void Update(double z_abs, double var_z_abs, double dt) noexcept;

  // Getters for the state and its covariance.
  double GetXAbs() const noexcept { return x_abs_; }
  double GetXVel() const noexcept { return x_vel_; }
  double GetCovAbsAbs() const noexcept { return p_abs_abs_; }
  double GetCovAbsVel() const noexcept { return p_abs_vel_; }
  double GetCovVelVel() const noexcept { return p_vel_vel_; }
};
