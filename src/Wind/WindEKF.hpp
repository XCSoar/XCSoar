/*
Copyright_License {

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

#ifndef WINDEKF_HPP
#define WINDEKF_HPP

#include "Engine/Navigation/SpeedVector.hpp"
#include "Math/Angle.hpp"
#include "Math/fixed.hpp"

struct NMEAInfo;
struct DerivedInfo;

class WindEKF {
  // constants/macros/typdefs
#define NUMX 3			
#define NUMW 3			// number of plant noise inputs, w is disturbance noise vector
#define NUMV 1			// number of measurements, v is the measurement noise vector
#define NUMU 2			// number of deterministic inputs, U is the input vector

  float F[NUMX][NUMX], G[NUMX][NUMW], H[NUMV][NUMX];	// linearized system matrices
  float P[NUMX][NUMX], X[NUMX];	// covariance matrix and state vector
  float Q[NUMW], R[NUMV];		// input noise and measurement noise variances
  float K[NUMX][NUMV];		// feedback gain matrix

public:
  WindEKF() {
    Init();
  }
  void Init();
  void StatePrediction(float gps_vel[2], float dT);
  void Correction(float dynamic_pressure, float gps_vel[2]);
  void CovariancePrediction(float dT);
  const float* get_state() const { return X; };

private:
  void SerialUpdate(float Z[NUMV], float Y[NUMV]);

  void RungeKutta(float U[NUMU], float dT);
  void StateEq(float U[NUMU], float Xdot[NUMX]);

  void LinearizeFG(float U[NUMU]);
  void MeasurementEq(float gps_vel[2], float Y[NUMV]);
  void LinearizeH(float gps_vel[2]);
};

class WindEKFGlue: protected WindEKF
{
  unsigned time_blackout;

public:
  struct Result {
    SpeedVector wind;
    int quality;

    Result() {}
    Result(int _quality):quality(_quality) {}
  };

  WindEKFGlue();

  Result Update(const NMEAInfo &basic, const DerivedInfo &derived);

  void reset() {
    time_blackout = (unsigned)-1;
  }

private:
  bool in_blackout(const unsigned time) const;
  void blackout(const unsigned time);
};

#endif
