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

#ifndef XCSOAR_KALMAN_FILTER_1D_HPP
#define XCSOAR_KALMAN_FILTER_1D_HPP

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
  KalmanFilter1d(double var_x_accel);
  KalmanFilter1d();

  // The following three methods reset the filter. All of them assign a huge
  // variance to the tracked absolute quantity and a var_x_accel_ variance to
  // its derivative, so the very next measurement will essentially be copied
  // directly into the filter. Still, we provide methods that allow you to
  // specify initial settings for the filter's tracked state.
  //
  // NOTE: "x_abs_value" is meant to connote the value of the absolute quantity
  // x, not the absolute value of x.
  void Reset();
  void Reset(double x_abs_value);
  void Reset(double x_abs_value, double x_vel_value);

  /**
   * Sets the variance of the acceleration noise input to the system model in
   * x units per second squared.
   */
  void SetAccelerationVariance(double var_x_accel) {
    var_x_accel_ = var_x_accel;
  }

  /**
   * Updates state given a direct sensor measurement of the absolute
   * quantity x, the variance of that measurement, and the interval
   * since the last measurement in seconds. This interval must be
   * greater than 0; for the first measurement after a Reset(), it's
   * safe to use 1.0.
   */
  void Update(double z_abs, double var_z_abs, double dt);

  // Getters for the state and its covariance.
  double GetXAbs() const { return x_abs_; }
  double GetXVel() const { return x_vel_; }
  double GetCovAbsAbs() const { return p_abs_abs_; }
  double GetCovAbsVel() const { return p_abs_vel_; }
  double GetCovVelVel() const { return p_vel_vel_; }
};

#endif
