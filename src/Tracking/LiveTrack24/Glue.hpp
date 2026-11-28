// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Settings.hpp"
#include "Client.hpp"
#include "time/PeriodClock.hpp"
#include "Geo/GeoPoint.hpp"
#include "net/AsyncTask.hpp"
#include "time/BrokenDateTime.hpp"

#include <atomic>
#include <chrono>

struct MoreData;
struct DerivedInfo;
class CurlGlobal;

namespace LiveTrack24 {

class Glue final {
  PeriodClock clock;

  Settings settings;

  /**
   * The device id the #Client points to.  Unlike #settings, it is
   * only written after the task has been cancelled.
   */
  decltype(Settings::device_id) device_id;

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
  bool flying = false;

  /**
   * Was the aircraft on the ground since Tick() last handled a
   * landing?  OnTimer() sets it also while requests are paused, so a
   * landing during a pause still ends the session before the next
   * flight is sent.  Cleared by Tick() in the I/O thread.
   */
  std::atomic<bool> landed{false};

  /**
   * No request before this time: #MIN_INTERVAL after the previous
   * request ended, or longer after the server's rate limit.  Written
   * by OnCompletion() in the I/O thread and by SetSettings() after
   * cancelling the task, possibly at the same time (see
   * PostponeNextRequest()); read by OnTimer().
   */
  std::atomic<std::chrono::steady_clock::time_point> next_request{};

  Net::AsyncTask task;

public:
  explicit Glue(CurlGlobal &curl) noexcept;

  void BeginShutdown() noexcept;

  void SetSettings(const Settings &_settings);
  void OnTimer(const MoreData &basic, const DerivedInfo &calculated);

protected:
  Co::InvokeTask Tick(Settings settings);
  void OnCompletion(std::exception_ptr error) noexcept;

private:
  /**
   * Move #next_request to @p t, unless it is already later.
   */
  void PostponeNextRequest(std::chrono::steady_clock::time_point t) noexcept;
};

} // namespace Livetrack24
