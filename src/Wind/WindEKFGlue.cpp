/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Wind/WindEKFGlue.hpp"
#include "Math/Angle.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"

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
      !basic.track_available || !basic.ground_speed_available ||
      !basic.airspeed_available || !basic.airspeed_real ||
      basic.true_airspeed < fixed_one) {
    reset();
    return Result(0);
  }

  // temporary manoeuvering, dont append this point
  unsigned time(basic.time);
  if ((fabs(derived.turn_rate) > fixed(20)) ||
      (fabs(basic.acceleration.g_load - fixed_one) > fixed(0.3))) {

    blackout(time);
    return Result(0);
  }

  if (in_blackout(time))
    return Result(0);

  // clear blackout
  blackout((unsigned)-1);

  fixed V = basic.true_airspeed;
  fixed dynamic_pressure = sqr(V);
  float gps_vel[2];
  const auto sc = basic.track.SinCos();
  const fixed gps_east = sc.first, gps_north = sc.second;
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
