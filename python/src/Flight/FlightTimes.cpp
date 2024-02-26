// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightTimes.hpp"
#include "DebugReplay.hpp"


void
Update(const MoreData &basic, const FlyingState &state,
       FlightTimeResult &result)
{
  if (!basic.time_available || !basic.date_time_utc.IsDatePlausible())
    return;

  if (state.flying && !result.takeoff_time.IsPlausible()) {
    result.takeoff_time = basic.GetDateTimeAt(state.takeoff_time);
    result.takeoff_location = state.takeoff_location;
  }

  if (!state.flying && result.takeoff_time.IsPlausible() &&
      !result.landing_time.IsPlausible()) {
    result.landing_time = basic.GetDateTimeAt(state.landing_time);
    result.landing_location = state.landing_location;
  }

  if (state.release_time.IsDefined() && !result.release_time.IsPlausible()) {
    result.release_time = basic.GetDateTimeAt(state.release_time);
    result.release_location = state.release_location;
  }
}

void
Update(const MoreData &basic, const DerivedInfo &calculated,
       FlightTimeResult &result)
{
  Update(basic, calculated.flight, result);
}

static void
Finish(const MoreData &basic, FlightTimeResult &result)
{
  if (!basic.time_available || !basic.date_time_utc.IsDatePlausible())
    return;

  if (result.takeoff_time.IsPlausible() && !result.landing_time.IsPlausible()) {
    result.landing_time = basic.date_time_utc;

    if (basic.location_available)
      result.landing_location = basic.location;
  }
}

bool
Run(DebugReplay &replay, FlightTimeResult &result)
{
  bool released = false;
  bool powered = false;

  GeoPoint last_location = GeoPoint::Invalid();
  constexpr Angle max_longitude_change = Angle::Degrees(30);
  constexpr Angle max_latitude_change = Angle::Degrees(1);

  replay.SetCalculated().Reset();
  replay.SetFlyingComputer().Reset();

  bool had_next;
  while ((had_next = replay.Next())) {
    const MoreData &basic = replay.Basic();

    Update(basic, replay.Calculated(), result);

    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable())
      continue;

    if (last_location.IsValid() &&
        ((last_location.latitude - basic.location.latitude).Absolute() > max_latitude_change ||
         (last_location.longitude - basic.location.longitude).Absolute() > max_longitude_change))
      /* there was an implausible warp, which is usually triggered by
         an invalid point declared "valid" by a bugged logger; if that
         happens, we stop the analysis, because the IGC file is
         obviously broken */
      break;

    last_location = basic.location;

    if (!released && replay.Calculated().flight.release_time.IsDefined()) {
      released = true;
    }

    if (replay.Calculated().flight.powered != powered) {
      powered = replay.Calculated().flight.powered;
      PowerState power_state;

      if (powered) {
        power_state.time = basic.GetDateTimeAt(replay.Calculated().flight.power_on_time);
        power_state.location = replay.Calculated().flight.power_on_location;
        power_state.state = PowerState::ON;
      } else {
        power_state.time = basic.GetDateTimeAt(replay.Calculated().flight.power_off_time);
        power_state.location = replay.Calculated().flight.power_off_location;
        power_state.state = PowerState::OFF;
      }

      result.power_states.push_back(power_state);
    }

    if (released && !replay.Calculated().flight.flying)
      /* the aircraft has landed, stop here */
      /* TODO: at some point, we might want to emit the analysis of
         all flights in this IGC file */
      break;
  }

  Update(replay.Basic(), replay.Calculated(), result);
  Finish(replay.Basic(), result);

  // landing detected or eof?
  if (had_next)
    return false;
  else
    return true;
}

void FlightTimes(DebugReplay &replay, std::vector<FlightTimeResult> &results)
{
  bool eof = false;
  while (!eof) {
    FlightTimeResult result;
    eof = Run(replay, result);

    if (result.takeoff_time.IsPlausible()
        && result.landing_time.IsPlausible()) {

      if (result.release_time.IsPlausible() &&
          result.release_time < result.takeoff_time)
        result.release_time = result.takeoff_time;

      results.push_back(result);
    }
  }
}
