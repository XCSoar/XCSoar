/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Glue.hpp"
#include "Settings.hpp"
#include "Queue.hpp"
#include "Assemble.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "net/State.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "util/ByteOrder.hxx"

#include <cassert>

static constexpr auto CLOUD_INTERVAL = std::chrono::minutes(1);

SkyLinesTracking::Glue::Glue(EventLoop &event_loop,
                             Handler *_handler)
  :client(event_loop, _handler),
   cloud_client(event_loop, _handler)
{
}

SkyLinesTracking::Glue::~Glue()
{
  delete queue;
}

inline bool
SkyLinesTracking::Glue::IsConnected() const
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
    return roaming;
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

  if (!IsConnected()) {
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

  if (!basic.location_available || !calculated.flight.flying)
    return;

  if (!IsConnected())
    return;

  if (cloud_clock.CheckAdvance(basic.time, CLOUD_INTERVAL))
    cloud_client.SendFix(basic);

  if (last_climb_time > basic.time)
    /* recover from time warp */
    last_climb_time = -1;

  constexpr double min_climb_duration = 30;
  constexpr double min_height_gain = 100;
  if (!calculated.circling &&
      calculated.climb_start_time >= 0 &&
      calculated.climb_start_time > last_climb_time &&
      calculated.cruise_start_time > calculated.climb_start_time + min_climb_duration &&
      calculated.cruise_start_altitude > calculated.climb_start_altitude + min_height_gain &&
      calculated.cruise_start_altitude_te > calculated.climb_start_altitude_te + min_height_gain) {
    /* we just stopped circling - send our thermal location to the
       XCSoar Cloud server */
    // TODO: use TE altitude?
    last_climb_time = calculated.cruise_start_time;

    double duration = calculated.cruise_start_time - calculated.climb_start_time;
    double height_gain = calculated.cruise_start_altitude - calculated.climb_start_altitude;
    double lift = height_gain / duration;

    cloud_client.SendThermal(ToBE32(uint32_t(basic.time * 1000)),
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
  if (basic.location_available && !basic.gps.real)
    /* disable in simulator/replay */
    return;

  if (client.IsConnected()) {
    SendFixes(basic);

    if (traffic_enabled &&
        traffic_clock.CheckAdvance(basic.clock, std::chrono::minutes(1)))
      client.SendTrafficRequest(true, true, near_traffic_enabled);
  }

  if (cloud_client.IsConnected()) {
    SendCloudFix(basic, calculated);

    if (thermal_enabled &&
        thermal_clock.CheckAdvance(basic.clock, std::chrono::minutes(1)))
      cloud_client.SendThermalRequest();
  }
}

void
SkyLinesTracking::Glue::SetSettings(const Settings &settings)
{
  thermal_enabled = settings.cloud.show_thermals;

  if (settings.cloud.enabled == TriState::TRUE && settings.cloud.key != 0) {
    cloud_client.SetKey(settings.cloud.key);
    if (!cloud_client.IsDefined()) {
      cloud_client.Open(*global_cares_channel, "cloud.xcsoar.net");
    }
  } else
    cloud_client.Close();

  if (!settings.enabled || settings.key == 0) {
    delete queue;
    queue = nullptr;
    client.Close();
    return;
  }

  client.SetKey(settings.key);

  interval = std::chrono::seconds(settings.interval);

  if (!client.IsDefined()) {
    client.Open(*global_cares_channel, "tracking.skylines.aero");
  }

  traffic_enabled = settings.traffic_enabled;
  near_traffic_enabled = settings.near_traffic_enabled;

  roaming = settings.roaming;
}
