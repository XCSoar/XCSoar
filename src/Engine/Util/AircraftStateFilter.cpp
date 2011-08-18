/* Copyright_License {

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
#include "AircraftStateFilter.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include <assert.h>

AircraftStateFilter::AircraftStateFilter(const fixed cutoff_wavelength)
  :x_diff_filter(fixed_zero), y_diff_filter(fixed_zero),
   alt_diff_filter(fixed_zero),
   x_low_pass(cutoff_wavelength), y_low_pass(cutoff_wavelength),
   alt_low_pass(cutoff_wavelength),
   x(fixed_zero), y(fixed_zero) {}

void
AircraftStateFilter::Reset(const AircraftState &state)
{
  last_state = state;

  x = fixed_zero;
  y = fixed_zero;

  v_x = fixed_zero;
  v_y = fixed_zero;
  v_alt = fixed_zero;

  x_low_pass.reset(fixed_zero);
  y_low_pass.reset(fixed_zero);
  alt_low_pass.reset(fixed_zero);
  x_diff_filter.reset(x, fixed_zero);
  y_diff_filter.reset(y, fixed_zero);
  alt_diff_filter.reset(state.altitude, fixed_zero);
}

void
AircraftStateFilter::Update(const AircraftState &state)
{
  fixed dt = state.time - last_state.time;

  if (negative(dt) || dt > fixed(60)) {
    Reset(state);
    return;
  }

  if (!positive(dt))
    return;

  GeoVector vec(last_state.location, state.location);

  const fixed MACH_1 = fixed_int_constant(343);
  if (vec.Distance > fixed(1000) || vec.Distance / dt > MACH_1) {
    Reset(state);
    return;
  }

  x += vec.Bearing.sin() * vec.Distance;
  y += vec.Bearing.cos() * vec.Distance;

  v_x = x_low_pass.update(x_diff_filter.update(x));
  v_y = y_low_pass.update(y_diff_filter.update(y));
  v_alt = alt_low_pass.update(alt_diff_filter.update(state.altitude));

  last_state = state;
}

fixed
AircraftStateFilter::GetSpeed() const
{
  return hypot(v_x, v_y);
}

Angle
AircraftStateFilter::GetBearing() const
{
  return Angle::from_xy(v_y, v_x).as_bearing();
}

bool
AircraftStateFilter::Design(const fixed cutoff_wavelength)
{
  bool ok = true;
  ok &= x_low_pass.design(cutoff_wavelength);
  ok &= y_low_pass.design(cutoff_wavelength);
  ok &= alt_low_pass.design(cutoff_wavelength);
  assert(ok);
  return ok;
}

AircraftState
AircraftStateFilter::GetPredictedState(const fixed &in_time) const
{
  AircraftState state_next = last_state;
  state_next.ground_speed = GetSpeed();
  state_next.vario = GetClimbRate();
  state_next.altitude = last_state.altitude + state_next.vario * in_time;
  state_next.location = GeoVector(state_next.ground_speed * in_time,
                                  GetBearing()).end_point(last_state.location);
  return state_next;
}
