// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Settings.hpp"
#include "Client.hpp"
#include "time/PeriodClock.hpp"
#include "Geo/GeoPoint.hpp"
#include "net/AsyncTask.hpp"
#include "time/BrokenDateTime.hpp"

struct MoreData;
struct DerivedInfo;
class CurlGlobal;

namespace LiveTrack24 {

class Glue final {
  PeriodClock clock;

  Settings settings;

  /** The current session; undefined while not flying. */
  Session session;
  unsigned packet_id;

  Client client;

  /**
   * The Unix UTC time stamp that was last submitted to the tracking
   * server.  This attribute is used to detect time warps.
   */
  std::chrono::system_clock::time_point last_timestamp{};

  BrokenDateTime date_time;
  GeoPoint location;
  int altitude;
  unsigned ground_speed;
  unsigned course;
  bool flying = false, last_flying;

  Net::AsyncTask task;

public:
  explicit Glue(CurlGlobal &curl) noexcept;

  void BeginShutdown() noexcept;

  void SetSettings(const Settings &_settings);
  void OnTimer(const MoreData &basic, const DerivedInfo &calculated);

protected:
  Co::InvokeTask Tick(Settings settings);
  void OnCompletion(std::exception_ptr error) noexcept;
};

} // namespace Livetrack24
