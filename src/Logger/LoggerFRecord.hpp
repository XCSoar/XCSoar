// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/GPSClock.hpp"
#include "NMEA/GPSState.hpp"

class TimeStamp;

class LoggerFRecord
{
  /* 4.5 minutes */
  static constexpr std::chrono::steady_clock::duration DEFAULT_UPDATE_TIME = std::chrono::seconds(270);
  static constexpr std::chrono::steady_clock::duration ACCELERATED_UPDATE_TIME = std::chrono::seconds(30);

  GPSClock clock;

  bool satellite_ids_available;
  int satellite_ids[GPSState::MAXSATELLITES];

public:
  /**
   * Returns true if the IGCWriter is supposed to write a new F record to
   * the IGC file or false if no update is needed.
   */
  bool Update(const GPSState &gps, TimeStamp time,
              bool nav_warning) noexcept;

  void Reset();

private:
  [[gnu::pure]]
  static bool IsBadSignal(const GPSState &gps) {
    return !gps.satellites_used_available || gps.satellites_used < 3;
  }

  [[gnu::pure]]
  bool CheckSatellitesChanged(const GPSState &gps) const;
};
