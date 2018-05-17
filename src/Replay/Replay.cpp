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

#include "Replay.hpp"
#include "IgcReplay.hpp"
#include "NmeaReplay.hpp"
#include "DemoReplayGlue.hpp"
#include "IO/FileLineReader.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Logger/Logger.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "CatmullRomInterpolator.hpp"
#include "Util/Clamp.hpp"

#include <stdexcept>

#include <assert.h>

void
Replay::Stop()
{
  if (replay == nullptr)
    return;

  Timer::Cancel();

  delete replay;
  replay = nullptr;

  delete cli;
  cli = nullptr;

  device_blackboard->StopReplay();

  if (logger != nullptr)
    logger->ClearBuffer();
}

void
Replay::Start(Path _path)
{
  assert(_path != nullptr);

  /* make sure the old AbstractReplay instance has cleaned up before
     creating a new one */
  Stop();

  path = _path;

  if (path.IsNull() || path.IsEmpty()) {
    replay = new DemoReplayGlue(task_manager);
  } else if (path.MatchesExtension(_T(".igc"))) {
    replay = new IgcReplay(std::make_unique<FileLineReaderA>(path));

    cli = new CatmullRomInterpolator(0.98);
    cli->Reset();
  } else {
    replay = new NmeaReplay(std::make_unique<FileLineReaderA>(path),
                            CommonInterface::GetSystemSettings().devices[0]);
  }

  if (logger != nullptr)
    logger->ClearBuffer();

  virtual_time = -1;
  fast_forward = -1;
  next_data.Reset();

  Timer::Schedule(100);
}

bool
Replay::Update()
{
  if (replay == nullptr)
    return false;

  if (time_scale <= 0) {
    /* replay is paused */
    /* to avoid a big fast-forward with the next
       PeriodClock::ElapsedUpdate() call below after unpausing, update
       the clock each time we're called while paused */
    clock.Update();
    return true;
  }

  const double old_virtual_time = virtual_time;

  if (virtual_time >= 0) {
    /* update the virtual time */
    assert(clock.IsDefined());

    if (fast_forward < 0) {
      virtual_time += clock.ElapsedUpdate() * time_scale / 1000;
    } else {
      clock.Update();

      virtual_time += 1;
      if (virtual_time >= fast_forward)
        fast_forward = -1;
    }
  } else {
    /* if we ever received a valid time from the AbstractReplay, then
       virtual_time must be initialised */
    assert(!next_data.time_available);
  }

  if (cli == nullptr || fast_forward >= 0) {
    if (next_data.time_available && virtual_time < next_data.time)
      /* still not time to use next_data */
      return true;

    {
      ScopeLock protect(device_blackboard->mutex);
      device_blackboard->SetReplayState() = next_data;
      device_blackboard->ScheduleMerge();
    }

    while (true) {
      if (!replay->Update(next_data)) {
        Stop();
        return false;
      }

      assert(!next_data.gps.real);

      if (next_data.time_available) {
        if (virtual_time < 0) {
          virtual_time = next_data.time;
          if (fast_forward >= 0)
            fast_forward += virtual_time;
          clock.Update();
          break;
        }

        if (next_data.time >= virtual_time)
          break;

        if (next_data.time < old_virtual_time) {
          /* time warp; that can happen on midnight wraparound during
             NMEA replay */
          virtual_time = next_data.time;
          break;
        }
      }
    }
  } else {
    while (cli->NeedData(virtual_time)) {
      if (!replay->Update(next_data)) {
        Stop();
        return false;
      }

      assert(!next_data.gps.real);

      if (next_data.time_available)
        cli->Update(next_data.time, next_data.location,
                    next_data.gps_altitude,
                    next_data.pressure_altitude);
    }

    if (virtual_time < 0) {
      virtual_time = cli->GetMaxTime();
      if (fast_forward >= 0)
        fast_forward += virtual_time;
      clock.Update();
    }

    const CatmullRomInterpolator::Record r = cli->Interpolate(virtual_time);
    const GeoVector v = cli->GetVector(virtual_time);

    NMEAInfo data = next_data;
    data.clock = virtual_time;
    data.alive.Update(data.clock);
    data.ProvideTime(virtual_time);
    data.location = r.location;
    data.location_available.Update(data.clock);
    data.ground_speed = v.distance;
    data.ground_speed_available.Update(data.clock);
    data.track = v.bearing;
    data.track_available.Update(data.clock);
    data.gps_altitude = r.gps_altitude;
    data.gps_altitude_available.Update(data.clock);
    data.ProvidePressureAltitude(r.baro_altitude);
    data.ProvideBaroAltitudeTrue(r.baro_altitude);

    {
      ScopeLock protect(device_blackboard->mutex);
      device_blackboard->SetReplayState() = data;
      device_blackboard->ScheduleMerge();
    }
  }

  return true;
}

void
Replay::OnTimer()
{
  if (!Update())
    return;

  unsigned schedule;
  if (time_scale <= 0)
    schedule = 1000;
  else if (fast_forward >= 0)
    schedule = 100;
  else if (virtual_time < 0 || !next_data.time_available)
    schedule = 500;
  else if (cli != nullptr)
    schedule = 1000;
  else {
    double delta_s = (next_data.time - virtual_time) / time_scale;
    int delta_ms = int(delta_s * 1000);
    schedule = Clamp(delta_ms, 100, 3000);
  }

  Timer::Schedule(schedule);
}
