// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"
#include "Validity.hpp"

/**
 * A container holding the aircraft current attitude state.
 */
struct AttitudeState
{
  /** Estimated bank angle */
  Angle bank_angle;

  /** Estimated pitch angle */
  Angle pitch_angle;

  /** Estimated heading */
  Angle heading;

  Validity bank_angle_available;
  Validity pitch_angle_available;
  Validity heading_available;

  /**
   * Invalidate all data held in this object.
   */
  constexpr void Reset() noexcept {
    bank_angle_available.Clear();
    pitch_angle_available.Clear();
    heading_available.Clear();
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const AttitudeState &add) noexcept;

  void Expire(TimeStamp now) noexcept;
};
