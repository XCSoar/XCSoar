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

#include "WindEKFGlue.hpp"
#include "Math/Angle.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"

void
WindEKFGlue::Reset()
{
  reset_pending = true;
  last_ground_speed_available.Clear();
  last_airspeed_available.Clear();
  i = 0;

  ResetBlackout();
}

static constexpr unsigned
CounterToQuality(unsigned i)
{
  return i >= 600u
    ? 4u
    : (i >= 120u
       ? 3u
       : (i >= 30u
          ? 2u
          : 1u));
}

WindEKFGlue::Result
WindEKFGlue::Update(const NMEAInfo &basic, const DerivedInfo &derived)
{
  // @todo accuracy: correct TAS for vertical speed if dynamic pullup

  // reset if flight hasnt started or airspeed instrument not available
  if (!derived.flight.flying) {
    Reset();
    return Result(0);
  }

  if (!basic.track_available || !basic.ground_speed_available ||
      !basic.airspeed_available || !basic.airspeed_real ||
      basic.true_airspeed < 1) {
    ResetBlackout();
    return Result(0);
  }

  if (last_ground_speed_available.FixTimeWarp(basic.ground_speed_available) ||
      last_airspeed_available.FixTimeWarp(basic.airspeed_available))
    /* time warp: start from scratch */
    Reset();

  if (!basic.ground_speed_available.Modified(last_ground_speed_available) ||
      !basic.airspeed_available.Modified(last_airspeed_available))
    /* no updated speed values from instrument, don't invoke WindEKF */
    return Result(0);

  last_ground_speed_available = basic.ground_speed_available;
  last_airspeed_available = basic.airspeed_available;

  // temporary manoeuvering, dont append this point
  if (derived.circling)
    /* reset the counter so the first wind estimate after circling
       ends is delayed for another 10 seconds */
    i = 0;

  unsigned time(basic.clock);
  if (derived.turn_rate.Absolute() > Angle::Degrees(20) ||
      (basic.acceleration.available &&
       basic.acceleration.real &&
       fabs(basic.acceleration.g_load - 1) > 0.3)) {

    SetBlackout(time);
    return Result(0);
  }

  if (InBlackout(time))
    return Result(0);

  // clear blackout
  ResetBlackout();

  auto V = basic.true_airspeed;
  float gps_vel[2];
  const auto sc = basic.track.SinCos();
  const auto gps_east = sc.first, gps_north = sc.second;
  gps_vel[0] = (float)(gps_east * basic.ground_speed);
  gps_vel[1] = (float)(gps_north * basic.ground_speed);

  if (reset_pending) {
    /* do the postponed WindEKF reset */
    reset_pending = false;
    ekf.Init();
  }

  ekf.Update(V, gps_vel);

  ++i;
  if (i % 10 != 0)
    return Result(0);

  const float* x = ekf.get_state();

  Result res;
  res.quality = CounterToQuality(i);
  res.wind = SpeedVector(-x[0], -x[1]);

  return res;
}
