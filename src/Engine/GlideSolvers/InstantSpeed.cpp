/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "GlideResult.hpp"
#include "Navigation/Aircraft.hpp"
#include "GlidePolar.hpp"

// Uncomment this if desired to have instantaneous speed revert to ground speed
// projected to target if above final glide.
//
// #define PIRKER_FINAL_GLIDE

double
GlideResult::InstantSpeed(const AircraftState& aircraft,
                          const GlideResult& leg,
                          const GlidePolar& glide_polar)
{
  // projection of ground speed to target
  const double v_a = (leg.vector.bearing-aircraft.track).cos() * aircraft.ground_speed;

  const auto d_mc = glide_polar.GetMC();

  // degenerate solution if no climb expected (mc=0), so just give instantaneous
  // projected ground speed to target
  if (d_mc <= 0)
    return v_a;

#ifdef PIRKER_FINAL_GLIDE
  if (altitude_difference> 0)
    return v_a;
#endif

  // cruise speed for current MC (m/s)
  const auto d_v = glide_polar.GetVBestLD();

  // sink rate at current MC speed (m/s)
  const auto d_s = glide_polar.GetSBestLD();

  double rho_c = 0, rho_g = 0;
  const double omega = aircraft.vario / d_mc;
  if (omega > 0) {
    const double rho = d_s / d_mc;
    rho_g = omega/(omega+rho);
  } else {
    rho_c = omega/(omega-1);
  }
  const double rho_a = 1 - rho_c - rho_g;
  return rho_a*v_a + rho_c*(-head_wind) + rho_g*(d_v-head_wind);
}
