// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindEKF.hpp"
#include "NMEA/Validity.hpp"
#include "Geo/SpeedVector.hpp"
#include "time/Stamp.hpp"

struct NMEAInfo;
struct DerivedInfo;

class WindEKFGlue
{
  /**
   * time to not add points after flight condition is false
   */
  static constexpr FloatDuration BLACKOUT_TIME = std::chrono::seconds{3};

  WindEKF ekf;

  /**
   * When this flag is true, then WindEKF::Init() should be called
   * before the WindEKF object is used.  This flag is used to postpone
   * the initialisation to the time when WindEKF is really needed, to
   * reduce the Reset() overhead when no airspeed indicator is
   * available.
   */
  bool reset_pending;

  /**
   * These attributes are used to check if updated values are
   * available since the last call.
   */
  Validity last_ground_speed_available, last_airspeed_available;

  /**
   * The number of samples we have fed into the #WindEKF.  This is
   * used to determine the quality.
   */
  unsigned i;

  TimeStamp time_blackout;

public:
  struct Result
  {
    SpeedVector wind;
    int quality;

    constexpr Result() noexcept {}
    constexpr Result(int _quality) noexcept:quality(_quality) {}
  };

  void Reset() noexcept;

  Result Update(const NMEAInfo &basic, const DerivedInfo &derived) noexcept;

private:
  void ResetBlackout() noexcept {
    time_blackout = TimeStamp::Undefined();
  }

  bool InBlackout(const TimeStamp time) const noexcept {
    return time < time_blackout;
  }

  void SetBlackout(const TimeStamp time) noexcept {
    time_blackout = time + BLACKOUT_TIME;
  }
};
