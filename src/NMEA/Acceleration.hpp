// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * State of acceleration of aircraft
 */
struct AccelerationState
{
  /**
   * Is G-load information available?
   * @see Gload
   */
  bool available;

  /**
   * Is the G-load information coming from a connected device (true) or
   * was it calculated by XCSoar (false)
   */
  bool real;

  /**
   * G-Load information of external device (if available)
   * or estimated (assuming balanced turn) 
   * @see AccelerationAvailable
   */
  double g_load;

  void Reset() {
    available = false;
  }

  void ProvideGLoad(double _g_load, bool _real=true) noexcept {
    g_load = _g_load;
    real = _real;
    available = true;
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const AccelerationState &add);
};
