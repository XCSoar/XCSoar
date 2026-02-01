// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once
#include "Math/Angle.hpp"

/**
 * State of angular rates of aircraft
 */
struct GyroscopeState
{
  /**
   * Is angular rate information available?
   */
  bool available;

  /**
   * Is the angular rate information coming from a connected device (true) or
   * was it calculated by XCSoar (false)
   */
  bool real;

  /**
   * Is the instrument housing the gyro sensor permanently mounted in the
   * aircraft, and do the sensor's three axes correspond to the aircraft's
   * three axes (true)? Or is the instrument mounted flexibly in the cockpit,
   * for example using a gooseneck (false)?
   * If in doubt, this flag should be set to false, as incorrect measurements
   * can lead to misinterpretations.
   */
  bool fixed_and_aligned;

  /**
   * angular rate information of external device (if available)
   * or estimated
   * In units of degrees per sec
   * In body frame axis(X forward, Y right, Z down)
   */
  Angle angular_rate_X; // roll, left wing up is positive
  Angle angular_rate_Y; // pitch, nose up is positive
  Angle angular_rate_Z; // yaw, right turn is positive

  void Reset()
  {
    available = false;
  }

  void ProvideAngularRates(Angle _angular_rate_X,
                           Angle _angular_rate_Y,
                           Angle _angular_rate_Z,
                           bool _fixed_and_aligned = false,
                           bool _real = true) noexcept
  {
    angular_rate_X    = _angular_rate_X;
    angular_rate_Y    = _angular_rate_Y;
    angular_rate_Z    = _angular_rate_Z;
    real              = _real;
    fixed_and_aligned = _fixed_and_aligned;
    available         = true;
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const GyroscopeState &add);
};
