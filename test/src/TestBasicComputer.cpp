// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/BasicComputer.hpp"
#include "Computer/Settings.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "TestUtil.hpp"

static MoreData
Sample(double time_s, double track_deg, double altitude,
       bool simulator) noexcept
{
  MoreData data;
  data.Reset();

  const auto time = TimeStamp{FloatDuration{time_s}};
  data.clock = time;
  data.alive.Update(data.clock);
  data.gps.simulator = simulator;
  data.gps.real = !simulator;

  data.time = time;
  data.time_available.Update(data.clock);
  data.track = Angle::Degrees(track_deg);
  data.track_available.Update(data.clock);
  data.ground_speed = 30;
  data.ground_speed_available.Update(data.clock);
  data.gps_altitude = altitude;
  data.gps_altitude_available.Update(data.clock);

  return data;
}

static void
Run(BasicComputer &computer, MoreData &data, const MoreData &last,
    const DerivedInfo &calculated, const ComputerSettings &settings) noexcept
{
  computer.Compute(data, last, last, calculated, settings);
}

int
main()
{
  plan_tests(17);

  BasicComputer computer;
  DerivedInfo calculated;
  calculated.flight.flying = false;
  calculated.wind_available.Clear();
  calculated.turn_rate_heading = Angle::Zero();
  calculated.sink_rate = 0;
  ComputerSettings settings{};

  auto prime = Sample(10, 0, 1000, true);
  Run(computer, prime, prime, calculated, settings);

  auto level = Sample(11, 0, 1000, true);
  Run(computer, level, prime, calculated, settings);
  ok1(level.attitude.heading_available);
  ok1(equals(level.attitude.heading, 0));
  ok1(level.attitude.bank_angle_available);
  ok1(level.attitude.pitch_angle_available);
  ok1(equals(level.attitude.bank_angle, 0));
  ok1(equals(level.attitude.pitch_angle, 0));

  /* 5° of track in one second at 30 m/s: about 15° of bank. */
  auto turning = Sample(12, 5, 1000, true);
  Run(computer, turning, level, calculated, settings);
  ok1(between(turning.attitude.bank_angle.Degrees(), 14.8, 15.1));
  ok1(equals(turning.attitude.pitch_angle, 0));

  /* Same fix again: merge is faster than the simulator tick. */
  auto held = Sample(12, 5, 1000, true);
  Run(computer, held, turning, calculated, settings);
  ok1(equals(held.attitude.bank_angle, turning.attitude.bank_angle));

  /* +3 m in one second, track steady: pitch up, bank back to level. */
  auto climbing = Sample(13, 5, 1003, true);
  Run(computer, climbing, turning, calculated, settings);
  ok1(between(climbing.attitude.pitch_angle.Degrees(), 5.6, 5.9));
  ok1(equals(climbing.attitude.bank_angle, 0));

  auto real = Sample(14, 10, 1010, false);
  const auto before = Sample(13, 5, 1003, false);
  Run(computer, real, before, calculated, settings);
  ok1(!real.attitude.bank_angle_available);
  ok1(!real.attitude.pitch_angle_available);

  /* IGC replay: same track change, no simulator flag. */
  BasicComputer replay_computer;
  auto replay_before = Sample(20, 0, 1000, false);
  replay_before.gps.replay = true;
  replay_before.gps.real = false;
  auto replay_turn = Sample(21, 5, 1000, false);
  replay_turn.gps.replay = true;
  replay_turn.gps.real = false;
  Run(replay_computer, replay_turn, replay_before, calculated, settings);
  ok1(between(replay_turn.attitude.bank_angle.Degrees(), 14.8, 15.1));
  ok1(equals(replay_turn.attitude.pitch_angle, 0));

  /* A log that already has attitude keeps it. */
  auto sensed = Sample(30, 0, 1000, false);
  sensed.gps.replay = true;
  sensed.gps.real = false;
  sensed.attitude.bank_angle = Angle::Degrees(7);
  sensed.attitude.bank_angle_available.Update(sensed.clock);
  sensed.attitude.pitch_angle = Angle::Degrees(-3);
  sensed.attitude.pitch_angle_available.Update(sensed.clock);
  auto sensed_next = sensed;
  sensed_next.time = TimeStamp{FloatDuration{31}};
  sensed_next.clock = sensed_next.time;
  sensed_next.track = Angle::Degrees(20);
  Run(replay_computer, sensed_next, sensed, calculated, settings);
  ok1(equals(sensed_next.attitude.bank_angle, 7));
  ok1(equals(sensed_next.attitude.pitch_angle, -3));

  return exit_status();
}
