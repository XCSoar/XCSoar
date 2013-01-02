/*
Copyright_License {

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

#ifndef WINDEKF_HPP
#define WINDEKF_HPP

#include "Math/fixed.hpp"

class WindEKF {
  // constants/macros/typdefs

  static constexpr unsigned NUMX = 3;

  /** number of plant noise inputs, w is disturbance noise vector */
  static constexpr unsigned NUMW = 3;

  /** number of measurements, v is the measurement noise vector */
  static constexpr unsigned NUMV = 1;

  /** number of deterministic inputs, U is the input vector */
  static constexpr unsigned NUMU = 2;

  /// linearized system matrices
  float F[NUMX][NUMX], G[NUMX][NUMW], H[NUMV][NUMX];

  /// covariance matrix and state vector
  float P[NUMX][NUMX], X[NUMX];

  /// input noise and measurement noise variances
  float Q[NUMW], R[NUMV];

  /// feedback gain matrix
  float K[NUMX][NUMV];

public:
  void Init();
  void StatePrediction(float gps_vel[2], float dT);
  void Correction(fixed dynamic_pressure, float gps_vel[2]);

  /**
   * Does the prediction step of the Kalman filter for the covariance matrix
   */
  void CovariancePrediction(float dT);
  const float* get_state() const { return X; };

private:
  /**
   * Does the update step of the Kalman filter for the covariance and estimate
   */
  void SerialUpdate(float Z[NUMV], float Y[NUMV]);

  /**
   * Does a 4th order Runge Kutta numerical integration step
   */
  void RungeKutta(float U[NUMU], float dT);
  void StateEq(float U[NUMU], float Xdot[NUMX]);

  void LinearizeFG(float U[NUMU]);
  void MeasurementEq(float gps_vel[2], float Y[NUMV]);
  void LinearizeH(float gps_vel[2]);
};

#endif
