/*
Copyright_License {

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

#include "WindEKF.hpp"
#include <math.h>

#define WIND_K0 1.0e-2f
#define WIND_K1 1.0e-5f

void
WindEKF::Update(const double airspeed, const float gps_vel[2])
{
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
