// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Client.hpp"
#include "time/GPSClock.hpp"
#include "time/Stamp.hpp"

struct DerivedInfo;

namespace SkyLinesTracking {

struct Settings;
class Queue;

class Glue {
  Client client;
  std::chrono::steady_clock::duration interval{};
  GPSClock clock;

  GPSClock traffic_clock;
  GPSClock thermal_clock;

  bool traffic_enabled = false;
  bool near_traffic_enabled = false;

  bool thermal_enabled = false;

  bool roaming = true;

  Queue *queue = nullptr;

  Client cloud_client;
  GPSClock cloud_clock;

  TimeStamp last_climb_time = TimeStamp::Undefined();

public:
  Glue(EventLoop &event_loop, Handler *_handler);
  ~Glue();

  void SetSettings(const Settings &settings);

  void Tick(const NMEAInfo &basic, const DerivedInfo &calculated);

  void RequestUserName(uint32_t user_id) {
    client.SendUserNameRequest(user_id);
  }

private:
  [[gnu::pure]]
  bool IsConnected() const;

  void SendFixes(const NMEAInfo &basic);
  void SendCloudFix(const NMEAInfo &basic, const DerivedInfo &calculated);
};

} /* namespace SkyLinesTracking */
