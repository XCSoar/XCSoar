// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaSwitchState.hpp"

#include <cstdint>

/**
 * State of external switch devices (esp Vega)
 */
struct SwitchState
{
  enum class FlightMode: uint8_t {
    UNKNOWN,
    CIRCLING,
    CRUISE,
  };

  enum class FlapPosition : uint8_t {
    UNKNOWN,
    POSITIVE,
    NEUTRAL,
    NEGATIVE,
    LANDING,
  };

  enum class UserSwitch : uint8_t {
    UNKNOWN,
    UP,
    MIDDLE,
    DOWN,
  };

  enum class AirbrakeState : uint8_t {
    UNKNOWN,
    LOCKED,
    NOT_LOCKED,
  };

  FlightMode flight_mode;
  FlapPosition flap_position;
  UserSwitch user_switch;
  AirbrakeState airbrake_state;

  VegaSwitchState vega;

  void Reset();

  void Complement(const SwitchState &add);
};
