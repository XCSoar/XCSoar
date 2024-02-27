// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct VegaSwitchState {
  enum InputBits {
    INPUT_FLAP_POSITIVE = 0,
    INPUT_FLAP_ZERO = 1,
    INPUT_FLAP_NEGATIVE = 2,
    INPUT_SPEED_COMMAND = 3,
    INPUT_GEAR_EXTENDED = 5,
    INPUT_AIRBRAKE_NOT_LOCKED = 6,
    INPUT_ACKNOWLEDGE = 8,
    INPUT_REPEAT = 9,
    INPUT_STALL = 20,
    INPUT_AIRBRAKE_LOCKED = 21,
    INPUT_USER_SWITCH_UP = 23,
    INPUT_USER_SWITCH_MIDDLE = 24,
    INPUT_USER_SWITCH_DOWN = 25,
  };

  enum OutputBits {
    OUTPUT_CIRCLING = 0,
    OUTPUT_FLAP_LANDING = 7,
  };

  static constexpr uint_least32_t INPUT_MASK_FLAP_POSITIVE = 1 << INPUT_FLAP_POSITIVE;
  static constexpr uint_least32_t INPUT_MASK_FLAP_ZERO = 1 << INPUT_FLAP_ZERO;
  static constexpr uint_least32_t INPUT_MASK_FLAP_NEGATIVE = 1 << INPUT_FLAP_NEGATIVE;
  static constexpr uint_least32_t INPUT_MASK_SC = 1 << INPUT_SPEED_COMMAND;
  static constexpr uint_least32_t INPUT_MASK_GEAR_EXTENDED = 1 << INPUT_GEAR_EXTENDED;
  static constexpr uint_least32_t INPUT_MASK_AIRBRAKE_NOT_LOCKED = 1 << INPUT_AIRBRAKE_NOT_LOCKED;
  static constexpr uint_least32_t INPUT_MASK_ACK = 1 << INPUT_ACKNOWLEDGE;
  static constexpr uint_least32_t INPUT_MASK_REP = 1 << INPUT_REPEAT;
  static constexpr uint_least32_t INPUT_MASK_AIRBRAKE_LOCKED = 1 << INPUT_AIRBRAKE_LOCKED;
  static constexpr uint_least32_t INPUT_MASK_USER_SWITCH_UP = 1 << INPUT_USER_SWITCH_UP;
  static constexpr uint_least32_t INPUT_MASK_USER_SWITCH_MIDDLE = 1 << INPUT_USER_SWITCH_MIDDLE;
  static constexpr uint_least32_t INPUT_MASK_USER_SWITCH_DOWN = 1 << INPUT_USER_SWITCH_DOWN;
  static constexpr uint_least32_t OUTPUT_MASK_CIRCLING = 1 << OUTPUT_CIRCLING;
  static constexpr uint_least32_t OUTPUT_MASK_FLAP_LANDING = 1 << OUTPUT_FLAP_LANDING;

  uint_least32_t inputs, outputs;

  constexpr bool IsDefined() const noexcept {
    return inputs != 0 || outputs != 0;
  }

  constexpr void Reset() noexcept {
    inputs = outputs = 0;
  }

  constexpr void Complement(const VegaSwitchState &add) noexcept {
    if (!IsDefined())
      *this = add;
  }

  constexpr bool GetFlapPositive() const noexcept {
    return inputs & INPUT_MASK_FLAP_POSITIVE;
  }

  constexpr bool GetFlapZero() const noexcept {
    return inputs & INPUT_MASK_FLAP_ZERO;
  }

  constexpr bool GetFlapNegative() const noexcept {
    return inputs & INPUT_MASK_FLAP_NEGATIVE;
  }

  constexpr bool GetSpeedCommand() const noexcept {
    return inputs & INPUT_MASK_SC;
  }

  constexpr bool GetGearExtended() const noexcept {
    return inputs & INPUT_MASK_GEAR_EXTENDED;
  }

  constexpr bool GetAirbrakeNotLocked() const noexcept {
    return inputs & INPUT_MASK_AIRBRAKE_NOT_LOCKED;
  }

  constexpr bool GetAcknowledge() const noexcept {
    return inputs & INPUT_MASK_ACK;
  }

  constexpr bool GetRepeat() const noexcept {
    return inputs & INPUT_MASK_REP;
  }

  constexpr bool GetAirbrakeLocked() const noexcept {
    return inputs & INPUT_MASK_AIRBRAKE_LOCKED;
  }

  constexpr bool GetUserSwitchUp() const noexcept {
    return inputs & INPUT_MASK_USER_SWITCH_UP;
  }

  constexpr bool GetUserSwitchMiddle() const noexcept {
    return inputs & INPUT_MASK_USER_SWITCH_MIDDLE;
  }

  constexpr bool GetUserSwitchDown() const noexcept {
    return inputs & INPUT_MASK_USER_SWITCH_DOWN;
  }

  constexpr bool GetCircling() const noexcept {
    return outputs & OUTPUT_MASK_CIRCLING;
  }

  constexpr bool GetFlapLanding() const noexcept {
    return outputs & OUTPUT_MASK_FLAP_LANDING;
  }
};
