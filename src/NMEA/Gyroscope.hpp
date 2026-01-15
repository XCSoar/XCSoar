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
                           bool _real = true) noexcept
  {
    angular_rate_X = _angular_rate_X;
    angular_rate_Y = _angular_rate_Y;
    angular_rate_Z = _angular_rate_Z;
    real           = _real;
    available      = true;
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const GyroscopeState &add);
};
