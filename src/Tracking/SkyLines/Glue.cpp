/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Net/State.hpp"
#include "Net/IPv4Address.hxx"

#ifdef HAVE_POSIX
#include "IO/Async/GlobalIOThread.hpp"
#endif

#include <assert.h>

static constexpr fixed CLOUD_INTERVAL = fixed(60);

SkyLinesTracking::Glue::Glue()
  :interval(0),
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
   traffic_enabled(false),
   near_traffic_enabled(false),
#endif
   roaming(true),
   queue(nullptr)
{
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  assert(io_thread != nullptr);
  client.SetIOThread(io_thread);
#endif
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
  assert(client.IsDefined());

  if (!basic.time_available) {
    clock.Reset();
    return;
  }

  if (!IsConnected()) {
    if (clock.CheckAdvance(basic.time, fixed(interval))) {
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
      if (!client.SendPacket(packet))
        break;

      queue->Pop();
      if (queue->IsEmpty()) {
        delete queue;
        queue = nullptr;
        break;
      }
    }

    return;
  } else if (clock.CheckAdvance(basic.time, fixed(interval)))
    client.SendFix(basic);
}

void
SkyLinesTracking::Glue::SendCloudFix(const NMEAInfo &basic)
{
  assert(cloud_client.IsDefined());

  if (!basic.time_available) {
    cloud_clock.Reset();
    return;
  }

  if (!basic.location_available)
    return;

  if (!IsConnected())
    return;

  if (cloud_clock.CheckAdvance(basic.time, CLOUD_INTERVAL))
    cloud_client.SendFix(basic);
}

void
SkyLinesTracking::Glue::Tick(const NMEAInfo &basic)
{
  if (basic.location_available && !basic.gps.real)
    /* disable in simulator/replay */
    return;

  if (client.IsDefined()) {
    SendFixes(basic);

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
    if (traffic_enabled && traffic_clock.CheckAdvance(basic.clock, fixed(60)))
      client.SendTrafficRequest(true, true, near_traffic_enabled);
#endif
  }

  if (cloud_client.IsDefined())
    SendCloudFix(basic);
}

void
SkyLinesTracking::Glue::SetSettings(const Settings &settings)
{
  if (settings.cloud_enabled == TriState::TRUE && settings.cloud_key != 0) {
    cloud_client.SetKey(settings.cloud_key);
    if (!cloud_client.IsDefined())
      // TODO: change hard-coded IP address to "cloud.xcsoar.net"
      cloud_client.Open(IPv4Address(78, 47, 110, 205,
                                    Client::GetDefaultPort()));
  } else
    cloud_client.Close();

  if (!settings.enabled || settings.key == 0) {
    delete queue;
    queue = nullptr;
    client.Close();
    return;
  }

  client.SetKey(settings.key);

  interval = settings.interval;

  if (!client.IsDefined())
    // TODO: fix hard-coded IP address:
    client.Open(IPv4Address(95, 128, 34, 172, Client::GetDefaultPort()));

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  traffic_enabled = settings.traffic_enabled;
  near_traffic_enabled = settings.near_traffic_enabled;
#endif

  roaming = settings.roaming;
}
