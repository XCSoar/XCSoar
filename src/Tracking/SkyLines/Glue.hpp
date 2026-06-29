// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Client.hpp"
#include "Tracking/SkyLines/TrafficExtensions.hpp"
#include "time/GPSClock.hpp"
#include "time/Stamp.hpp"
#include "util/StaticString.hxx"

struct CloudSettings;
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

  bool skylines_roaming = true;

  Queue *queue = nullptr;

  Client cloud_client;
  GPSClock cloud_clock;
  GPSClock cloud_traffic_clock;

  bool cloud_show_traffic = true;
  bool cloud_roaming = true;

  StaticString<64> cloud_host;

  unsigned cloud_port = 0;

  TimeStamp last_climb_time = TimeStamp::Undefined();

public:
  Glue(EventLoop &event_loop, Handler *_handler);
  ~Glue();

  void SetSettings(const Settings &skylines_settings,
                   const CloudSettings &cloud_settings);

  void BeginShutdown() noexcept;

  void Tick(const NMEAInfo &basic, const DerivedInfo &calculated);

  void RequestUserName(uint32_t user_id) {
    if ((user_id & OGN_PILOT_ID_MASK) != 0) {
      if (cloud_client.IsConnected())
        cloud_client.SendUserNameRequest(user_id);
    } else if (client.IsConnected())
      client.SendUserNameRequest(user_id);
  }

private:
  [[gnu::pure]]
  bool IsNetConnected(bool roaming_allowed) const;

  void SendFixes(const NMEAInfo &basic);
  void SendCloudFix(const NMEAInfo &basic, const DerivedInfo &calculated);
};

} /* namespace SkyLinesTracking */
