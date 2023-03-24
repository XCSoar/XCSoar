// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Blackboard/DeviceBlackboard.hpp"
#include "Protection.hpp"
#include "Simulator.hpp"
#include "RadioFrequency.hpp"

#include <algorithm>

/**
 * Initializes the DeviceBlackboard
 */
DeviceBlackboard::DeviceBlackboard() noexcept
{
  // Clear the gps_info and calculated_info
  gps_info.Reset();
  calculated_info.Reset();

  // Set GPS assumed time to system time
  gps_info.UpdateClock();
  gps_info.date_time_utc = BrokenDateTime::NowUTC();
  gps_info.time = TimeStamp{gps_info.date_time_utc.DurationSinceMidnight()};

  std::fill(per_device_data.begin(), per_device_data.end(), gps_info);

  real_data = simulator_data = replay_data = gps_info;

  simulator.Init(simulator_data);

  real_clock.Reset();
  replay_clock.Reset();
}

/**
 * Sets the location and altitude to loc and alt
 *
 * Called at startup when no gps data available yet
 * @param loc New location
 * @param alt New altitude
 */
void
DeviceBlackboard::SetStartupLocation(const GeoPoint &loc,
                                     const double alt) noexcept
{
  const std::lock_guard lock{mutex};

  if (Calculated().flight.flying)
    return;

  for (auto &i : per_device_data)
    if (!i.location_available)
      i.SetFakeLocation(loc, alt);

  if (!real_data.location_available)
    real_data.SetFakeLocation(loc, alt);

  if (is_simulator()) {
    simulator_data.SetFakeLocation(loc, alt);
    simulator.Touch(simulator_data);
  }

  ScheduleMerge();
}

/**
 * Stops the replay
 */
void
DeviceBlackboard::StopReplay() noexcept
{
  const std::lock_guard lock{mutex};

  replay_data.Reset();

  ScheduleMerge();
}

void
DeviceBlackboard::ProcessSimulation() noexcept
{
  if (!is_simulator())
    return;

  const std::lock_guard lock{mutex};

  simulator.Process(simulator_data);
  ScheduleMerge();
}

void
DeviceBlackboard::SetSimulatorLocation(const GeoPoint &location) noexcept
{
  const std::lock_guard lock{mutex};
  NMEAInfo &basic = simulator_data;

  simulator.Touch(basic);
  basic.track = location.Bearing(basic.location).Reciprocal();
  basic.location = location;

  ScheduleMerge();
}

/**
 * Sets the GPS speed and indicated airspeed to val
 *
 * not in use
 * @param val New speed
 */
void
DeviceBlackboard::SetSpeed(double val) noexcept
{
  const std::lock_guard lock{mutex};
  NMEAInfo &basic = simulator_data;

  simulator.Touch(basic);
  basic.ground_speed = val;

  ScheduleMerge();
}

/**
 * Sets the TrackBearing to val
 *
 * not in use
 * @param val New TrackBearing
 */
void
DeviceBlackboard::SetTrack(Angle val) noexcept
{
  const std::lock_guard lock{mutex};
  simulator.Touch(simulator_data);
  simulator_data.track = val.AsBearing();

  ScheduleMerge();
}

/**
 * Sets the altitude and barometric altitude to val
 *
 * not in use
 * @param val New altitude
 */
void
DeviceBlackboard::SetAltitude(double val) noexcept
{
  const std::lock_guard lock{mutex};
  NMEAInfo &basic = simulator_data;

  simulator.Touch(basic);
  basic.gps_altitude = val;

  ScheduleMerge();
}

void
DeviceBlackboard::ExpireWallClock() noexcept
{
  const std::lock_guard lock{mutex};
  if (!Basic().alive)
    return;

  bool modified = false;
  for (auto &basic : per_device_data) {
    if (!basic.alive)
      continue;

    basic.ExpireWallClock();
    if (!basic.alive)
      modified = true;
  }

  if (modified)
    ScheduleMerge();
}

void
DeviceBlackboard::ScheduleMerge() noexcept
{
  TriggerMergeThread();
}

void
DeviceBlackboard::Merge() noexcept
{
  NMEAInfo &basic = SetBasic();

  real_data.Reset();
  for (auto &basic : per_device_data) {
    if (!basic.alive)
      continue;

    basic.UpdateClock();
    basic.Expire();
    real_data.Complement(basic);
  }

  real_clock.Normalise(real_data);

  if (replay_data.alive) {
    replay_data.Expire();
    basic = replay_data;

    /* WrapClock operates on the replay_data copy to avoid feeding
       back BrokenDate modifications to the NMEA parser, as this would
       trigger its time warp checks */
    replay_clock.Normalise(basic);
  } else if (simulator_data.alive) {
    simulator_data.UpdateClock();
    simulator_data.Expire();
    basic = simulator_data;
  } else {
    basic = real_data;
  }
}
