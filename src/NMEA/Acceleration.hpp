// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Validity.hpp"
#include "time/Stamp.hpp"

/**
 * State of acceleration of aircraft
 */
struct AccelerationState
{
  /**
   * A monotonic wall clock time, in seconds, with an undefined
   * reference.  This may get updated even if the device doesn't send
   * any data.  It is used to update and check the #Validity
   * attributes in this struct.
   */
  TimeStamp clock;

  /**
   * Is G-load information available?
   * @see Gload
   */
  Validity available;

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
    available.Clear();
  }

  void ProvideGLoad(double _g_load, bool _real=true) noexcept {
    g_load = _g_load;
    real = _real;
    clock = TimeStamp{std::chrono::steady_clock::now().time_since_epoch()};
    available.Update(clock);
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const AccelerationState &add);
};
