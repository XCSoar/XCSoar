/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "IO/Async/AsioThread.hpp"
#include "IO/Async/GlobalAsioThread.hpp"

#include <assert.h>

SkyLinesTracking::Glue::Glue(Handler *_handler)
  :client(*asio_thread, _handler)
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
  } else if (clock.CheckAdvance(basic.time, interval))
    client.SendFix(basic);
}

void
SkyLinesTracking::Glue::Tick(const NMEAInfo &basic)
{
  if (!client.IsDefined())
    return;

  if (basic.location_available && !basic.gps.real)
    /* disable in simulator/replay */
    return;

  SendFixes(basic);

  if (traffic_enabled && traffic_clock.CheckAdvance(basic.clock, 60))
    client.SendTrafficRequest(true, true, near_traffic_enabled);
}

void
SkyLinesTracking::Glue::SetSettings(const Settings &settings)
{
  if (!settings.enabled || settings.key == 0) {
    client.Close();
    return;
  }

  client.SetKey(settings.key);

  interval = settings.interval;

  if (!client.IsDefined()) {
    // TODO: fix hard-coded IP address:
    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address_v4::from_string("95.128.34.172"),
                                            Client::GetDefaultPort());
    client.Open(endpoint);
  }

  traffic_enabled = settings.traffic_enabled;
  near_traffic_enabled = settings.near_traffic_enabled;

  roaming = settings.roaming;
}
