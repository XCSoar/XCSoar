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

#ifndef XCSOAR_VEGA_SWITCH_STATE_HPP
#define XCSOAR_VEGA_SWITCH_STATE_HPP

struct VegaSwitchState {
  static constexpr unsigned INVALID = -1;

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

  static constexpr unsigned INPUT_MASK_FLAP_POSITIVE = 1 << INPUT_FLAP_POSITIVE;
  static constexpr unsigned INPUT_MASK_FLAP_ZERO = 1 << INPUT_FLAP_ZERO;
  static constexpr unsigned INPUT_MASK_FLAP_NEGATIVE = 1 << INPUT_FLAP_NEGATIVE;
  static constexpr unsigned INPUT_MASK_SC = 1 << INPUT_SPEED_COMMAND;
  static constexpr unsigned INPUT_MASK_GEAR_EXTENDED = 1 << INPUT_GEAR_EXTENDED;
  static constexpr unsigned INPUT_MASK_AIRBRAKE_NOT_LOCKED = 1 << INPUT_AIRBRAKE_NOT_LOCKED;
  static constexpr unsigned INPUT_MASK_ACK = 1 << INPUT_ACKNOWLEDGE;
  static constexpr unsigned INPUT_MASK_REP = 1 << INPUT_REPEAT;
  static constexpr unsigned INPUT_MASK_AIRBRAKE_LOCKED = 1 << INPUT_AIRBRAKE_LOCKED;
  static constexpr unsigned INPUT_MASK_USER_SWITCH_UP = 1 << INPUT_USER_SWITCH_UP;
  static constexpr unsigned INPUT_MASK_USER_SWITCH_MIDDLE = 1 << INPUT_USER_SWITCH_MIDDLE;
  static constexpr unsigned INPUT_MASK_USER_SWITCH_DOWN = 1 << INPUT_USER_SWITCH_DOWN;
  static constexpr unsigned OUTPUT_MASK_CIRCLING = 1 << OUTPUT_CIRCLING;
  static constexpr unsigned OUTPUT_MASK_FLAP_LANDING = 1 << OUTPUT_FLAP_LANDING;

  unsigned inputs, outputs;

  constexpr bool IsDefined() const {
    return inputs != 0 || outputs != 0;
  }

  void Reset() {
    inputs = outputs = 0;
  }

  void Complement(const VegaSwitchState &add) {
    if (!IsDefined())
      *this = add;
  }

  constexpr bool GetFlapPositive() const {
    return inputs & INPUT_MASK_FLAP_POSITIVE;
  }

  constexpr bool GetFlapZero() const {
    return inputs & INPUT_MASK_FLAP_ZERO;
  }

  constexpr bool GetFlapNegative() const {
    return inputs & INPUT_MASK_FLAP_NEGATIVE;
  }

  constexpr bool GetSpeedCommand() const {
    return inputs & INPUT_MASK_SC;
  }

  constexpr bool GetGearExtended() const {
    return inputs & INPUT_MASK_GEAR_EXTENDED;
  }

  constexpr bool GetAirbrakeNotLocked() const {
    return inputs & INPUT_MASK_AIRBRAKE_NOT_LOCKED;
  }

  constexpr bool GetAcknowledge() const {
    return inputs & INPUT_MASK_ACK;
  }

  constexpr bool GetRepeat() const {
    return inputs & INPUT_MASK_REP;
  }

  constexpr bool GetAirbrakeLocked() const {
    return inputs & INPUT_MASK_AIRBRAKE_LOCKED;
  }

  constexpr bool GetUserSwitchUp() const {
    return inputs & INPUT_MASK_USER_SWITCH_UP;
  }

  constexpr bool GetUserSwitchMiddle() const {
    return inputs & INPUT_MASK_USER_SWITCH_MIDDLE;
  }

  constexpr bool GetUserSwitchDown() const {
    return inputs & INPUT_MASK_USER_SWITCH_DOWN;
  }

  constexpr bool GetCircling() const {
    return outputs & OUTPUT_MASK_CIRCLING;
  }

  constexpr bool GetFlapLanding() const {
    return outputs & OUTPUT_MASK_FLAP_LANDING;
  }
};

#endif
