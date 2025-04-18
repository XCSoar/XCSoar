// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Settings.hpp"
#include "Client.hpp"
#include "time/PeriodClock.hpp"
#include "Geo/GeoPoint.hpp"
#include "co/InjectTask.hxx"
#include "time/BrokenDateTime.hpp"

struct MoreData;
struct DerivedInfo;
class CurlGlobal;

namespace LiveTrack24 {

class Glue final {
  struct State
  {
    LiveTrack24::SessionID session_id;
    unsigned packet_id;

    void ResetSession() {
      session_id = 0;
    }

    bool HasSession() {
      return session_id != 0;
    }
  };

  PeriodClock clock;

  Settings settings;

  State state;

  Client client;

  /**
   * The Unix UTC time stamp that was last submitted to the tracking
   * server.  This attribute is used to detect time warps.
   */
  std::chrono::system_clock::time_point last_timestamp{};

  BrokenDateTime date_time;
  GeoPoint location;
  unsigned altitude;
  unsigned ground_speed;
  Angle track;
  bool flying = false, last_flying;

  Co::InjectTask inject_task;

public:
  explicit Glue(CurlGlobal &curl) noexcept;

  void SetSettings(const Settings &_settings);
  void OnTimer(const MoreData &basic, const DerivedInfo &calculated);

protected:
  Co::InvokeTask Tick(Settings settings);
  void OnCompletion(std::exception_ptr error) noexcept;
};

} // namespace Livetrack24
