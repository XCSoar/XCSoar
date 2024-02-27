// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SwitchState.hpp"

void
SwitchState::Reset() noexcept
{
  flight_mode = FlightMode::UNKNOWN;
  flap_position = FlapPosition::UNKNOWN;
  user_switch = UserSwitch::UNKNOWN;
  airbrake_state = AirbrakeState::UNKNOWN;
  vega.Reset();
}

void
SwitchState::Complement(const SwitchState &add) noexcept
{
  if (flight_mode == FlightMode::UNKNOWN)
    flight_mode = add.flight_mode;

  if (flap_position == FlapPosition::UNKNOWN)
    flap_position = add.flap_position;

  if (user_switch == UserSwitch::UNKNOWN)
    user_switch = add.user_switch;

  if (airbrake_state == AirbrakeState::UNKNOWN)
    airbrake_state = add.airbrake_state;

  vega.Complement(add.vega);
}
