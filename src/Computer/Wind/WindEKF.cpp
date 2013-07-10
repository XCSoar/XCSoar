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

#include "WindEKF.hpp"

#include <stdint.h>

void
WindEKF::StatePrediction(float gps_vel[2], float dT)
{
  float U[2];

  U[0] = gps_vel[0];
  U[1] = gps_vel[1];

  // EKF prediction step
  LinearizeFG(U);
  RungeKutta(U, dT);
}

void
WindEKF::Correction(fixed dynamic_pressure, float gps_vel[2])
{
  float Z[1], Y[1];

  Z[0] = (float)dynamic_pressure;

  // EKF correction step
  LinearizeH(gps_vel);
  MeasurementEq(gps_vel, Y);
  SerialUpdate(Z, Y);
}

void
WindEKF::CovariancePrediction(float dT)
{
  //  Output, Pnew, overwrites P, the input covariance
  //  Pnew = (I+F*T)*P*(I+F*T)' + T^2*G*Q*G'
  //  Q is the discrete time covariance of process noise
  //  Q is vector of the diagonal for a square matrix with
  //    dimensions equal to the number of disturbance noise variables

  float Dummy[NUMX][NUMX], dTsq;

  //  Pnew = (I+F*T)*P*(I+F*T)' + T^2*G*Q*G' = T^2[(P/T + F*P)*(I/T + F') + G*Q*G')]

  dTsq = dT * dT;

  // Calculate Dummy = (P/T +F*P)
  for (unsigned i = 0; i < NUMX; i++) {
    for (unsigned j = 0; j < NUMX; j++) {
      Dummy[i][j] = P[i][j] / dT;
      for (unsigned k = 0; k < NUMX; k++)
        Dummy[i][j] += F[i][k] * P[k][j];
    }
  }

  // Calculate Pnew = Dummy/T + Dummy*F' + G*Qw*G'
  for (unsigned i = 0; i < NUMX; i++) {
    for (unsigned j = i; j < NUMX; j++) {	// Use symmetry, ie only find upper triangular
      P[i][j] = Dummy[i][j] / dT;
      for (unsigned k = 0; k < NUMX; k++)
        P[i][j] += Dummy[i][k] * F[j][k];	// P = Dummy/T + Dummy*F'
      for (unsigned k = 0; k < NUMW; k++)
        P[i][j] += Q[k] * G[i][k] * G[j][k];	// P = Dummy/T + Dummy*F' + G*Q*G'
      P[j][i] = P[i][j] = P[i][j] * dTsq;	// Pnew = T^2*P and fill in lower triangular;
    }
  }
}

inline void
WindEKF::SerialUpdate(float Z[NUMV], float Y[NUMV])
{
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

inline void
WindEKF::RungeKutta(float U[NUMU], float dT)
{
  //  Output, Xnew, is written over X
  //  NOTE the algorithm assumes time invariant state equations and
  //    constant inputs over integration step

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

inline void
WindEKF::StateEq(float U[NUMU], float Xdot[NUMX])
{
  // assume wind and sf are constant
  Xdot[0] = 0;
  Xdot[1] = 0;
  Xdot[2] = 0;
}

inline void
WindEKF::LinearizeFG(float U[NUMU])
{
  F[0][0] = 1;
  F[1][1] = 1;
  F[2][2] = 1;

  G[0][0] = 1;
  G[1][1] = 1;
  G[2][2] = 1;
}

inline void
WindEKF::MeasurementEq(float gps_vel[2], float Y[NUMV])
{
  float sf = X[2];
  float dx = gps_vel[0]-X[0];
  float dy = gps_vel[1]-X[1];
  Y[0] = sf*(dx*dx+dy*dy);
}

inline void
WindEKF::LinearizeH(float gps_vel[2])
{
  float sf = X[2];
  float dx = gps_vel[0]-X[0];
  float dy = gps_vel[1]-X[1];

  H[0][0] = -2*sf*dx;
  H[0][1] = -2*sf*dy;
  H[0][2] = dx*dx+dy*dy;
}

void
WindEKF::Init()
{
  for (unsigned i = 0; i < NUMX; i++) {
    for (unsigned j = 0; j < NUMX; j++) {
      P[i][j] = 0; // zero all terms
    }
  }

  P[0][0] = P[1][1] = 10.0;	// initial wind speed variance ((m/s)^2)
  P[2][2] = 1.0e-1; // initial sf variance

  X[0] = X[1] = 0;	// initial wind speed (m/s)
  X[2] = 1; // initial scale factor

  Q[0] = Q[1] = 0.1;
  Q[2] = 1.0e-4;

  R[0] = 10.0;	// dynamic pressure noise variance ((m/s)^2)
}
