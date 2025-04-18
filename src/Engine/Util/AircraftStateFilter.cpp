// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AircraftStateFilter.hpp"
#include "Geo/GeoVector.hpp"

#include <cassert>

AircraftStateFilter::AircraftStateFilter(const double cutoff_wavelength) noexcept
  :x_diff_filter(0), y_diff_filter(0),
   alt_diff_filter(0),
   x_low_pass(cutoff_wavelength), y_low_pass(cutoff_wavelength),
   alt_low_pass(cutoff_wavelength),
   x(0), y(0) {}

void
AircraftStateFilter::Reset(const AircraftState &state) noexcept
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
AircraftStateFilter::Update(const AircraftState &state) noexcept
{
  auto dt = state.time - last_state.time;

  if (dt.count() < 0 || dt > std::chrono::minutes{1}) {
    Reset(state);
    return;
  }

  if (dt.count() <= 0)
    return;

  GeoVector vec(last_state.location, state.location);

  constexpr double MACH_1 = 343;
  if (vec.distance > 1000 || vec.distance / dt.count() > MACH_1) {
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
AircraftStateFilter::GetSpeed() const noexcept
{
  return hypot(v_x, v_y);
}

Angle
AircraftStateFilter::GetBearing() const noexcept
{
  return Angle::FromXY(v_y, v_x).AsBearing();
}

bool
AircraftStateFilter::Design(const FloatDuration cutoff_wavelength) noexcept
{
  bool ok = true;
  ok &= x_low_pass.Design(cutoff_wavelength.count());
  ok &= y_low_pass.Design(cutoff_wavelength.count());
  ok &= alt_low_pass.Design(cutoff_wavelength.count());
  assert(ok);
  return ok;
}

AircraftState
AircraftStateFilter::GetPredictedState(const FloatDuration in_time) const noexcept
{
  AircraftState state_next = last_state;
  state_next.ground_speed = GetSpeed();
  state_next.vario = GetClimbRate();
  state_next.altitude = last_state.altitude + state_next.vario * in_time.count();
  state_next.location = GeoVector(state_next.ground_speed * in_time.count(),
                                  GetBearing()).EndPoint(last_state.location);
  return state_next;
}
