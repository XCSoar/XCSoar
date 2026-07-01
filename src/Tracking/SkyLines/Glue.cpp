// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "Settings.hpp"
#include "Tracking/CloudSettings.hpp"
#include "Queue.hpp"
#include "Assemble.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "net/State.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "util/ByteOrder.hxx"
#include "util/Compiler.h"
#include "util/EnvParser.hpp"
#include "util/StringCompare.hxx"
#include "LogFile.hpp"

#include <cassert>

using namespace std::chrono;

static constexpr auto CLOUD_INTERVAL = minutes(1);

SkyLinesTracking::Glue::Glue(EventLoop &event_loop,
                             Handler *_handler)
  :client(event_loop, _handler, TrafficSource::SKYLINES),
   cloud_client(event_loop, _handler, TrafficSource::CLOUD)
{
}

SkyLinesTracking::Glue::~Glue()
{
  BeginShutdown();
}

void
SkyLinesTracking::Glue::BeginShutdown() noexcept
{
  client.Close();
  cloud_client.Close();
  delete queue;
  queue = nullptr;
}

inline bool
SkyLinesTracking::Glue::IsNetConnected(bool roaming_allowed) const
{
  switch (GetNetState()) {
  case NetState::UNKNOWN:
    /* we don't know if we have an internet connection - be
       optimistic, and assume everything's ok */
    return true;

  case NetState::DISCONNECTED:
    return false;

  case NetState::CONNECTED:
    return true;

  case NetState::ROAMING:
    return roaming_allowed;
  }

  assert(false);
  gcc_unreachable();
}

inline void
SkyLinesTracking::Glue::SendFixes(const NMEAInfo &basic)
{
  assert(client.IsConnected());

  if (!basic.time_available) {
    clock.Reset();
    return;
  }

  if (!IsNetConnected(skylines_roaming)) {
    if (clock.CheckAdvance(basic.time, interval)) {
      /* queue the packet, send it later */
      if (queue == nullptr)
        queue = new Queue();
      queue->Push(ToFix(client.GetKey(), basic));
    }

    return;
  }

  if (queue != nullptr) {
    /* send queued fix packets, 8 at a time */
    unsigned n = 8;
    while (n-- > 0) {
      const auto &packet = queue->Peek();
      client.SendPacket(packet);

      queue->Pop();
      if (queue->IsEmpty()) {
        delete queue;
        queue = nullptr;
        break;
      }
    }

    return;
  } else if (clock.CheckAdvance(basic.time, interval))
    client.SendFix(basic);
}

void
SkyLinesTracking::Glue::SendCloudFix(const NMEAInfo &basic,
                                     const DerivedInfo &calculated)
{
  assert(cloud_client.IsConnected());

  if (!basic.time_available) {
    cloud_clock.Reset();
    return;
  }

  if (!basic.location_available)
    return;

  if (!calculated.flight.flying)
    return;

  if (!IsNetConnected(cloud_roaming)) {
    if (GetEnvBool("XCS_CLOUD_DEBUG"))
      LogFmt("Cloud: FIX skipped (network/roaming gate)");
    return;
  }

  if (cloud_clock.CheckAdvance(basic.time, CLOUD_INTERVAL)) {
    cloud_client.SendFix(basic);
    if (GetEnvBool("XCS_CLOUD_DEBUG"))
      LogFmt("Cloud: sent FIX flying={}", calculated.flight.flying);
  }

  if (last_climb_time > basic.time)
    /* recover from time warp */
    last_climb_time = TimeStamp::Undefined();

  constexpr FloatDuration min_climb_duration = seconds{30};
  constexpr double min_height_gain = 100;
  if (!calculated.circling &&
      calculated.climb_start_time.IsDefined() &&
      calculated.climb_start_time > last_climb_time &&
      calculated.cruise_start_time > calculated.climb_start_time + min_climb_duration &&
      calculated.cruise_start_altitude > calculated.climb_start_altitude + min_height_gain &&
      calculated.cruise_start_altitude_te > calculated.climb_start_altitude_te + min_height_gain) {
    /* we just stopped circling - send our thermal location to the
       XCSoar Cloud server */
    // TODO: use TE altitude?
    last_climb_time = calculated.cruise_start_time;

    const auto duration = calculated.cruise_start_time - calculated.climb_start_time;
    double height_gain = calculated.cruise_start_altitude - calculated.climb_start_altitude;
    double lift = height_gain / duration.count();

    cloud_client.SendThermal(ToBE32(basic.time.Cast<::duration<uint32_t, milliseconds::period>>().count()),
                             calculated.climb_start_location,
                             iround(calculated.climb_start_altitude),
                             calculated.cruise_start_location,
                             iround(calculated.cruise_start_altitude),
                             (double)lift);
  }
}

void
SkyLinesTracking::Glue::Tick(const NMEAInfo &basic,
                             const DerivedInfo &calculated)
{
  const bool simulator =
    basic.location_available && !basic.gps.real;
  const bool cloud_debug = GetEnvBool("XCS_CLOUD_DEBUG");

  if (simulator && !cloud_debug)
    /* disable in simulator/replay */
    return;

  if (client.IsConnected() && !simulator) {
    SendFixes(basic);

    if (traffic_enabled &&
        traffic_clock.CheckAdvance(basic.clock, minutes(1)))
      client.SendTrafficRequest(true, true, near_traffic_enabled);
  }

  if (cloud_client.IsConnected()) {
    SendCloudFix(basic, calculated);

    if (cloud_show_traffic && basic.location_available &&
        (calculated.flight.flying || cloud_debug) &&
        cloud_traffic_clock.CheckAdvance(basic.clock, minutes(1))) {
      /* near=true: cloud server returns nearby cloud + OGN traffic */
      cloud_client.SendTrafficRequest(false, false, true);
      if (cloud_debug)
        LogFmt("Cloud: sent TRAFFIC_REQUEST (near)");
    }

    if (thermal_enabled &&
        thermal_clock.CheckAdvance(basic.clock, minutes(1)))
      cloud_client.SendThermalRequest();
  }
}

void
SkyLinesTracking::Glue::SetSettings(const Settings &skylines_settings,
                                    const CloudSettings &cloud_settings)
{
  thermal_enabled = cloud_settings.show_thermals;
  cloud_show_traffic = cloud_settings.show_traffic;
  cloud_roaming = cloud_settings.roaming;

  if (cloud_settings.enabled == TriState::TRUE && cloud_settings.key != 0) {
    cloud_client.SetKey(cloud_settings.key);

    const char *host = cloud_settings.HostCStr();
    const unsigned port = cloud_settings.EffectivePort();
    const bool endpoint_changed =
      !StringIsEqual(cloud_host, host) || cloud_port != port;
    if (endpoint_changed && cloud_client.IsDefined())
      cloud_client.Close();

    if (!cloud_client.IsDefined()) {
      cloud_host = host;
      cloud_port = port;
      cloud_client.Open(*global_cares_channel, host, port);
      if (GetEnvBool("XCS_CLOUD_DEBUG"))
        LogFmt("Cloud: opening {}:{} (simulator override enabled)",
               host, port);
    }
  } else {
    cloud_client.Close();
    cloud_host.clear();
    cloud_port = 0;
  }

  if (!skylines_settings.enabled || skylines_settings.key == 0) {
    delete queue;
    queue = nullptr;
    client.Close();
    return;
  }

  client.SetKey(skylines_settings.key);

  interval = seconds(skylines_settings.interval);

  if (!client.IsDefined()) {
    client.Open(*global_cares_channel, "tracking.skylines.aero");
  }

  traffic_enabled = skylines_settings.traffic_enabled;
  near_traffic_enabled = skylines_settings.near_traffic_enabled;

  skylines_roaming = skylines_settings.roaming;
}
