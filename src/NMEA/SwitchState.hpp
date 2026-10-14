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
  enum class FlightMode: uint_least8_t {
    UNKNOWN,
    CIRCLING,
    CRUISE,
  };

  enum class FlapPosition : uint_least8_t {
    UNKNOWN,
    POSITIVE,
    NEUTRAL,
    NEGATIVE,
    LANDING,
  };

  enum class UserSwitch : uint_least8_t {
    UNKNOWN,
    UP,
    MIDDLE,
    DOWN,
  };

  enum class AirbrakeState : uint_least8_t {
    UNKNOWN,
    LOCKED,
    NOT_LOCKED,
  };

  FlightMode flight_mode;
  FlapPosition flap_position;
  UserSwitch user_switch;
  AirbrakeState airbrake_state;

  VegaSwitchState vega;

  /**
   * Cruise/climb report for the page.  A Vega page follows the
   * speed-command input.  Every other device uses flight_mode.
   */
  [[nodiscard]]
  constexpr FlightMode SpeedCommandPage() const noexcept {
    if (vega.IsDefined())
      return vega.GetSpeedCommand()
        ? FlightMode::CRUISE
        : FlightMode::CIRCLING;
    return flight_mode;
  }

  void Reset() noexcept;

  void Complement(const SwitchState &add) noexcept;
};
