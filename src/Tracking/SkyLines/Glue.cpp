/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "NMEA/Info.hpp"

#ifdef HAVE_POSIX
#include "IO/Async/GlobalIOThread.hpp"
#endif

#include <assert.h>

SkyLinesTracking::Glue::Glue()
  :interval(0), clock(fixed(10))
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  , traffic_clock(fixed(60)), traffic_enabled(false)
#endif
{
#ifdef HAVE_POSIX
  assert(io_thread != nullptr);
  client.SetIOThread(io_thread);
#endif
}

void
SkyLinesTracking::Glue::Tick(const NMEAInfo &basic)
{
  if (!client.IsDefined())
    return;

  if (!basic.time_available) {
    clock.Reset();
#ifdef HAVE_SKYLINES_TRACKING_HANDLER
    traffic_clock.Reset();
#endif
    return;
  }

  if (clock.CheckAdvance(basic.time))
    client.SendFix(basic);

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  if (traffic_enabled && traffic_clock.CheckAdvance(basic.time))
    client.SendTrafficRequest(true, true);
#endif
}

void
SkyLinesTracking::Glue::SetSettings(const Settings &settings)
{
  client.SetKey(settings.key);

  if (interval != settings.interval) {
    interval = settings.interval;
    clock = GPSClock(fixed(std::max(settings.interval, 1u)));
  }

  if (!settings.enabled)
    client.Close();
  else if (!client.IsDefined())
    // TODO: fix hard-coded IP address:
    client.Open("78.47.50.46");

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  traffic_enabled = settings.traffic_enabled;
#endif
}
