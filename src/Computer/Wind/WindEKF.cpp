// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindEKF.hpp"

#include <cassert>
#include <cmath>

static constexpr float WIND_K0 = 1.0e-2f;
static constexpr float WIND_K1 = 1.0e-5f;

void
WindEKF::Update(const double airspeed, const float gps_vel[2])
{
  assert(!std::isnan(airspeed));
  assert(!std::isnan(gps_vel[0]));
  assert(!std::isnan(gps_vel[1]));

  // airsp = sf * | gps_v - wind_v |
  const float dx = gps_vel[0]-X[0];
  const float dy = gps_vel[1]-X[1];
  const float mag = hypotf(dx, dy);

  const float K[3] = {
    -X[2]*dx/mag*k,
    -X[2]*dy/mag*k,
    mag*WIND_K1
  };
  k += 0.01f*(WIND_K0-k);
  
  // measurement equation
  const float Error = (float)airspeed - X[2]*mag;
  X[0] += K[0] * Error;
  X[1] += K[1] * Error;
  X[2] += K[2] * Error;
  
  // limit values
  if (X[2]<0.5f) {
    X[2] = 0.5f;
  } else if (X[2]>1.5f) {
    X[2] = 1.5f;
  }
}

void
WindEKF::Init()
{
  k = WIND_K0*4;

  X[0] = X[1] = 0;	// initial wind speed (m/s)
  X[2] = 1;             // initial scale factor
}
