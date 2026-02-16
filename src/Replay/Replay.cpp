// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Replay.hpp"
#include "IgcReplay.hpp"
#include "NmeaReplay.hpp"
#include "DemoReplayGlue.hpp"
#include "io/FileLineReader.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Logger/Logger.hpp"
#include "Interface.hpp"
#include "CatmullRomInterpolator.hpp"
#include "time/Cast.hxx"

#include <algorithm> // for std::clamp()
#include <cassert>
#include <stdexcept>

void
Replay::Stop()
{
  if (replay == nullptr)
    return;

  timer.Cancel();

  delete replay;
  replay = nullptr;

  delete cli;
  cli = nullptr;

  device_blackboard.StopReplay();

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

  if (path == nullptr || path.empty()) {
    replay = new DemoReplayGlue(device_blackboard, task_manager);
  } else if (path.EndsWithIgnoreCase(".igc")) {
    replay = new IgcReplay(std::make_unique<FileLineReaderA>(path));

    cli = new CatmullRomInterpolator(FloatDuration{0.98});
    cli->Reset();
  } else {
    replay = new NmeaReplay(std::make_unique<FileLineReaderA>(path),
                            CommonInterface::GetSystemSettings().devices[0]);
  }

  if (logger != nullptr)
    logger->ClearBuffer();

  virtual_time = TimeStamp::Undefined();
  fast_forward = TimeStamp::Undefined();
  next_data.Reset();

  timer.Schedule(std::chrono::milliseconds(100));
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

  const auto old_virtual_time = virtual_time;

  if (virtual_time.IsDefined()) {
    /* update the virtual time */
    assert(clock.IsDefined());

    if (!fast_forward.IsDefined()) {
      virtual_time += clock.ElapsedUpdate() * time_scale;
    } else {
      clock.Update();

      virtual_time += std::chrono::seconds{1};
      if (virtual_time >= fast_forward)
        fast_forward = TimeStamp::Undefined();
    }
  } else {
    /* if we ever received a valid time from the AbstractReplay, then
       virtual_time must be initialised */
    assert(!next_data.time_available);
  }

  if (cli == nullptr || fast_forward.IsDefined()) {
    if (next_data.time_available && virtual_time < next_data.time)
      /* still not time to use next_data */
      return true;

    {
      const std::lock_guard lock{device_blackboard.mutex};
      device_blackboard.SetReplayState() = next_data;
      device_blackboard.ScheduleMerge();
    }

    while (true) {
      if (!replay->Update(next_data)) {
        Stop();
        return false;
      }

      assert(!next_data.gps.real);

      if (next_data.time_available) {
        if (!virtual_time.IsDefined()) {
          virtual_time = next_data.time;
          if (fast_forward.IsDefined())
            fast_forward = virtual_time + fast_forward.ToDuration();
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

    if (!virtual_time.IsDefined()) {
      virtual_time = cli->GetMaxTime();
      if (fast_forward.IsDefined())
        fast_forward = virtual_time + fast_forward.ToDuration();
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
      const std::lock_guard lock{device_blackboard.mutex};
      device_blackboard.SetReplayState() = data;
      device_blackboard.ScheduleMerge();
    }
  }

  return true;
}

void
Replay::OnTimer()
{
  if (!Update())
    return;

  std::chrono::steady_clock::duration schedule;
  if (time_scale <= 0)
    schedule = std::chrono::seconds(1);
  else if (fast_forward.IsDefined())
    schedule = std::chrono::milliseconds(100);
  else if (!virtual_time.IsDefined() || !next_data.time_available)
    schedule = std::chrono::milliseconds(500);
  else if (cli != nullptr)
    schedule = std::chrono::seconds(1);
  else {
    constexpr std::chrono::steady_clock::duration lower = std::chrono::milliseconds(100);
    constexpr std::chrono::steady_clock::duration upper = std::chrono::seconds(3);
    const FloatDuration delta_s((next_data.time - virtual_time) / time_scale);
    const auto delta = std::chrono::duration_cast<std::chrono::steady_clock::duration>(delta_s);
    schedule = std::clamp(delta, lower, upper);
  }

  timer.Schedule(schedule);
}
