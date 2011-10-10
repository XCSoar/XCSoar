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

#include "Wind/WindEKF.hpp"
#include "Math/FastMath.h"
#include "Math/Angle.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"

#include <math.h>

//  *************  Exposed Functions ****************
//  *************************************************

void WindEKF::StatePrediction(float gps_vel[2], float dT)
{
  float U[2];

  U[0] = gps_vel[0];
  U[1] = gps_vel[1];

  // EKF prediction step
  LinearizeFG(U);
  RungeKutta(U, dT);
}

void WindEKF::Correction(fixed dynamic_pressure, float gps_vel[2])
{
  float Z[1], Y[1];

  Z[0] = (float)dynamic_pressure;

  // EKF correction step
  LinearizeH(gps_vel);
  MeasurementEq(gps_vel, Y);
  SerialUpdate(Z, Y);
}

//  *************  CovariancePrediction *************
//  Does the prediction step of the Kalman filter for the covariance matrix
//  Output, Pnew, overwrites P, the input covariance
//  Pnew = (I+F*T)*P*(I+F*T)' + T^2*G*Q*G'
//  Q is the discrete time covariance of process noise
//  Q is vector of the diagonal for a square matrix with
//    dimensions equal to the number of disturbance noise variables
//  ************************************************

void WindEKF::CovariancePrediction(float dT)
{
  float Dummy[NUMX][NUMX], dTsq;
  uint8_t i, j, k;

  //  Pnew = (I+F*T)*P*(I+F*T)' + T^2*G*Q*G' = T^2[(P/T + F*P)*(I/T + F') + G*Q*G')]

  dTsq = dT * dT;

  for (i = 0; i < NUMX; i++)	// Calculate Dummy = (P/T +F*P)
    for (j = 0; j < NUMX; j++) {
      Dummy[i][j] = P[i][j] / dT;
      for (k = 0; k < NUMX; k++)
        Dummy[i][j] += F[i][k] * P[k][j];
    }
  for (i = 0; i < NUMX; i++)	// Calculate Pnew = Dummy/T + Dummy*F' + G*Qw*G'
    for (j = i; j < NUMX; j++) {	// Use symmetry, ie only find upper triangular
      P[i][j] = Dummy[i][j] / dT;
      for (k = 0; k < NUMX; k++)
        P[i][j] += Dummy[i][k] * F[j][k];	// P = Dummy/T + Dummy*F'
      for (k = 0; k < NUMW; k++)
        P[i][j] += Q[k] * G[i][k] * G[j][k];	// P = Dummy/T + Dummy*F' + G*Q*G'
      P[j][i] = P[i][j] = P[i][j] * dTsq;	// Pnew = T^2*P and fill in lower triangular;
    }
}

//  *************  SerialUpdate *******************
//  Does the update step of the Kalman filter for the covariance and estimate
//  Outputs are Xnew & Pnew, and are written over P and X
//  Z is actual measurement, Y is predicted measurement
//  Xnew = X + K*(Z-Y), Pnew=(I-K*H)*P,
//    where K=P*H'*inv[H*P*H'+R]
//  NOTE the algorithm assumes R (measurement covariance matrix) is diagonal
//    i.e. the measurment noises are uncorrelated.
//  It therefore uses a serial update that requires no matrix inversion by
//    processing the measurements one at a time.
//  Algorithm - see Grewal and Andrews, "Kalman Filtering,2nd Ed" p.121 & p.253
//            - or see Simon, "Optimal State Estimation," 1st Ed, p.150
//  ************************************************

void WindEKF::SerialUpdate(float Z[NUMV], float Y[NUMV])
{
  float HP[NUMX], HPHR, Error;
  uint8_t i, j, k, m;

  for (m = 0; m < NUMV; m++) {

    for (j = 0; j < NUMX; j++) {	// Find Hp = H*P
      HP[j] = 0;
      for (k = 0; k < NUMX; k++)
        HP[j] += H[m][k] * P[k][j];
    }
    HPHR = R[m];	// Find  HPHR = H*P*H' + R
    for (k = 0; k < NUMX; k++)
      HPHR += HP[k] * H[m][k];

    assert(HPHR>0.0); // JMW prevent potential crash
    if (HPHR <= 0.0) continue;

    for (k = 0; k < NUMX; k++)
      K[k][m] = HP[k] / HPHR;	// find K = HP/HPHR

    for (i = 0; i < NUMX; i++) {	// Find P(m)= P(m-1) + K*HP
      for (j = i; j < NUMX; j++)
        P[i][j] = P[j][i] =
          P[i][j] - K[i][m] * HP[j];
    }

    Error = Z[m] - Y[m];
    for (i = 0; i < NUMX; i++)	// Find X(m)= X(m-1) + K*Error
      X[i] = X[i] + K[i][m] * Error;

  }
}

//  *************  RungeKutta **********************
//  Does a 4th order Runge Kutta numerical integration step
//  Output, Xnew, is written over X
//  NOTE the algorithm assumes time invariant state equations and
//    constant inputs over integration step
//  ************************************************

void WindEKF::RungeKutta(float U[NUMU], float dT)
{
  float dT2 =
    dT / 2, K1[NUMX], K2[NUMX], K3[NUMX], K4[NUMX], Xlast[NUMX];
  uint8_t i;

  for (i = 0; i < NUMX; i++)
    Xlast[i] = X[i];	// make a working copy

  StateEq(U, K1);	// k1 = f(x,u)
  for (i = 0; i < NUMX; i++)
    X[i] = Xlast[i] + dT2 * K1[i];
  StateEq(U, K2);	// k2 = f(x+0.5*dT*k1,u)
  for (i = 0; i < NUMX; i++)
    X[i] = Xlast[i] + dT2 * K2[i];
  StateEq(U, K3);	// k3 = f(x+0.5*dT*k2,u)
  for (i = 0; i < NUMX; i++)
    X[i] = Xlast[i] + dT * K3[i];
  StateEq(U, K4);	// k4 = f(x+dT*k3,u)

  // Xnew  = X + dT*(k1+2*k2+2*k3+k4)/6
  for (i = 0; i < NUMX; i++)
    X[i] =
      Xlast[i] + dT * (K1[i] + 2 * K2[i] + 2 * K3[i] +
                       K4[i]) / 6;
}

//  *************  Model Specific Stuff  ***************************
//  ***  StateEq, MeasurementEq, LinerizeFG, and LinearizeH ********
//
//  State Variables = [Pos Vel Quaternion GyroBias NO-AccelBias]
//  Deterministic Inputs = [AngularVel Accel]
//  Disturbance Noise = [GyroNoise AccelNoise GyroRandomWalkNoise NO-AccelRandomWalkNoise]
//
//  Measurement Variables = [Pos Vel BodyFrameMagField Altimeter]
//  Inputs to Measurement = [EarthFrameMagField]
//
//  Notes: Pos and Vel in earth frame
//  AngularVel and Accel in body frame
//  MagFields are unit vectors
//  Xdot is output of StateEq()
//  F and G are outputs of LinearizeFG(), all elements not set should be zero
//  y is output of OutputEq()
//  H is output of LinearizeH(), all elements not set should be zero
//  ************************************************

void WindEKF::StateEq(float U[NUMU], float Xdot[NUMX])
{
  // assume wind and sf are constant
  Xdot[0] = 0;
  Xdot[1] = 0;
  Xdot[2] = 0;
}

void WindEKF::LinearizeFG(float U[NUMU])
{
  F[0][0] = 1;
  F[1][1] = 1;
  F[2][2] = 1;

  G[0][0] = 1;
  G[1][1] = 1;
  G[2][2] = 1;
}

void WindEKF::MeasurementEq(float gps_vel[2], float Y[NUMV])
{
  float sf = X[2];
  float dx = gps_vel[0]-X[0];
  float dy = gps_vel[1]-X[1];
  Y[0] = sf*(dx*dx+dy*dy);
}

void WindEKF::LinearizeH(float gps_vel[2])
{
  float sf = X[2];
  float dx = gps_vel[0]-X[0];
  float dy = gps_vel[1]-X[1];

  H[0][0] = -2*sf*dx;
  H[0][1] = -2*sf*dy;
  H[0][2] = dx*dx+dy*dy;
}

void WindEKF::Init()
{
  for (int i = 0; i < NUMX; i++) {
    for (int j = 0; j < NUMX; j++) {
      P[i][j] = 0; // zero all terms
    }
  }

  P[0][0] = P[1][1] = 10.0;	// initial wind speed variance ((m/s)^2)
  P[2][2] = 1.0e-4; // initial sf variance

  X[0] = X[1] = 0;	// initial wind speed (m/s)
  X[2] = 1; // initial scale factor

  Q[0] = Q[1] = 0.1;
  Q[2] = 1.0e-4;

  R[0] = 10.0;	// dynamic pressure noise variance ((m/s)^2)
}

/**
 * time to not add points after flight condition is false
 */
#define BLACKOUT_TIME 3

WindEKFGlue::Result
WindEKFGlue::Update(const NMEAInfo &basic, const DerivedInfo &derived)
{
  // @todo accuracy: correct TAS for vertical speed if dynamic pullup

  // reset if flight hasnt started or airspeed instrument not available
  if (!derived.flight.flying ||
      !basic.airspeed_available || !basic.airspeed_real ||
      basic.true_airspeed < fixed_one) {
    reset();
    return Result(0);
  }

  // temporary manoeuvering, dont append this point
  if ((fabs(derived.turn_rate) > fixed(20)) ||
      (fabs(basic.acceleration.g_load - fixed_one) > fixed(0.3))) {

    blackout(basic.time);
    return Result(0);
  }

  if (in_blackout(basic.time))
    return Result(0);

  // clear blackout
  blackout((unsigned)-1);

  fixed V = basic.true_airspeed;
  fixed dynamic_pressure = sqr(V);
  float gps_vel[2];
  fixed gps_east, gps_north;
  basic.track.SinCos(gps_east, gps_north);
  gps_vel[0] = (float)(gps_east * basic.ground_speed);
  gps_vel[1] = (float)(gps_north * basic.ground_speed);

  float dT = 1.0;

  StatePrediction(gps_vel, dT);
  Correction(dynamic_pressure, gps_vel);
  // CovariancePrediction(dT);
  const float* x = get_state();

  Result res;
  static int j=0;
  j++;
  if (j%10==0)
    res.quality = 1;
  else
    res.quality = 0;

  res.wind = SpeedVector(fixed(-x[0]), fixed(-x[1]));

  return res;
}

void
WindEKFGlue::blackout(const unsigned time)
{
  time_blackout = time;
}

bool
WindEKFGlue::in_blackout(const unsigned time) const
{
  return (time < time_blackout + BLACKOUT_TIME);
}

WindEKFGlue::WindEKFGlue()
{
  reset();
  Init();
}
