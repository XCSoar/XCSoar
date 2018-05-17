/* Copyright_License {

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

#include "AircraftStateFilter.hpp"
#include "Geo/GeoVector.hpp"

#include <assert.h>

AircraftStateFilter::AircraftStateFilter(const double cutoff_wavelength)
  :x_diff_filter(0), y_diff_filter(0),
   alt_diff_filter(0),
   x_low_pass(cutoff_wavelength), y_low_pass(cutoff_wavelength),
   alt_low_pass(cutoff_wavelength),
   x(0), y(0) {}

void
AircraftStateFilter::Reset(const AircraftState &state)
{
  last_state = state;

  x = 0;
  y = 0;

  v_x = 0;
  v_y = 0;
  v_alt = 0;

  x_low_pass.Reset(0);
  y_low_pass.Reset(0);
  alt_low_pass.Reset(0);
  x_diff_filter.Reset(x, 0);
  y_diff_filter.Reset(y, 0);
  alt_diff_filter.Reset(state.altitude, 0);
}

void
AircraftStateFilter::Update(const AircraftState &state)
{
  auto dt = state.time - last_state.time;

  if (dt < 0 || dt > 60) {
    Reset(state);
    return;
  }

  if (dt <= 0)
    return;

  GeoVector vec(last_state.location, state.location);

  constexpr double MACH_1 = 343;
  if (vec.distance > 1000 || vec.distance / dt > MACH_1) {
    Reset(state);
    return;
  }

  x += vec.bearing.sin() * vec.distance;
  y += vec.bearing.cos() * vec.distance;

  v_x = x_low_pass.Update(x_diff_filter.Update(x));
  v_y = y_low_pass.Update(y_diff_filter.Update(y));
  v_alt = alt_low_pass.Update(alt_diff_filter.Update(state.altitude));

  last_state = state;
}

double
AircraftStateFilter::GetSpeed() const
{
  return hypot(v_x, v_y);
}

Angle
AircraftStateFilter::GetBearing() const
{
  return Angle::FromXY(v_y, v_x).AsBearing();
}

bool
AircraftStateFilter::Design(const double cutoff_wavelength)
{
  bool ok = true;
  ok &= x_low_pass.Design(cutoff_wavelength);
  ok &= y_low_pass.Design(cutoff_wavelength);
  ok &= alt_low_pass.Design(cutoff_wavelength);
  assert(ok);
  return ok;
}

AircraftState
AircraftStateFilter::GetPredictedState(const double in_time) const
{
  AircraftState state_next = last_state;
  state_next.ground_speed = GetSpeed();
  state_next.vario = GetClimbRate();
  state_next.altitude = last_state.altitude + state_next.vario * in_time;
  state_next.location = GeoVector(state_next.ground_speed * in_time,
                                  GetBearing()).EndPoint(last_state.location);
  return state_next;
}
