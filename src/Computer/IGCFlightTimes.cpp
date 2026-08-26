// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGCFlightTimes.hpp"
#include "FlyingComputer.hpp"
#include "IGC/IGCExtensions.hpp"
#include "IGC/IGCFix.hpp"
#include "IGC/IGCParser.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "NMEA/FlyingState.hpp"
#include "NMEA/Info.hpp"
#include "Units/System.hpp"
#include "io/FileLineReader.hpp"
#include "time/FloatDuration.hxx"
#include "time/WrapClock.hpp"

namespace {

/** Minimum slow time before takeoff / after landing for a “complete” log. */
constexpr FloatDuration MIN_MARGIN{std::chrono::seconds{5}};

} // namespace

bool
DetectIGCFlightTimes(Path path, double takeoff_speed,
                     IGCFlightTimesResult &result) noexcept
{
  result = {};

  if (takeoff_speed <= 0)
    takeoff_speed = DEFAULT_IGC_TAKEOFF_SPEED;

  FlyingComputer flying_computer;
  FlyingState flight;
  flying_computer.Reset();
  flight.Reset();

  WrapClock wrap_clock;
  wrap_clock.Reset();

  IGCExtensions extensions;
  extensions.clear();

  NMEAInfo basic;
  basic.Reset();

  bool saw_takeoff = false;
  bool saw_landing = false;
  bool forced_landing = false;
  GeoPoint last_location = GeoPoint::Invalid();
  TimeStamp last_time = TimeStamp::Undefined();
  FloatDuration slow_before{};
  FloatDuration slow_after{};

  try {
    FileLineReaderA reader(path);
    char *line;
    while ((line = reader.ReadLine()) != nullptr) {
      if (line[0] == 'H') {
        BrokenDate date;
        if (IGCParseDateRecord(line, date)) {
          basic.ProvideDate(date);
          basic.time_available.Clear();
        }
        continue;
      }

      if (line[0] == 'I') {
        IGCParseExtensions(line, extensions);
        continue;
      }

      if (line[0] != 'B')
        continue;

      IGCFix fix;
      if (!IGCParseFix(line, extensions, fix) || !fix.gps_valid)
        continue;

      if (basic.time_available && basic.date_time_utc.hour >= 23 &&
          fix.time.hour == 0)
        basic.date_time_utc.IncrementDay();

      basic.clock = basic.time =
        TimeStamp{fix.time.DurationSinceMidnight()};
      basic.time_available.Update(basic.clock);
      basic.date_time_utc.hour = fix.time.hour;
      basic.date_time_utc.minute = fix.time.minute;
      basic.date_time_utc.second = fix.time.second;
      basic.alive.Update(basic.clock);

      basic.location = fix.location;
      basic.location_available.Update(basic.clock);

      if (fix.gps_altitude != 0) {
        basic.gps_altitude = fix.gps_altitude;
        basic.gps_altitude_available.Update(basic.clock);
      } else
        basic.gps_altitude_available.Clear();

      if (fix.pressure_altitude != 0) {
        basic.ProvidePressureAltitude(fix.pressure_altitude);
        basic.ProvideBaroAltitudeTrue(fix.pressure_altitude);
      } else {
        basic.pressure_altitude_available.Clear();
        basic.baro_altitude_available.Clear();
      }

      wrap_clock.Normalise(basic);

      if (fix.gsp >= 0) {
        basic.ground_speed = Units::ToSysUnit(fix.gsp,
                                              Unit::KILOMETER_PER_HOUR);
        basic.ground_speed_available.Update(basic.clock);
      } else if (last_location.IsValid() && last_time.IsDefined() &&
                 basic.time > last_time) {
        const auto dt = basic.time - last_time;
        if (dt.count() > 0) {
          basic.ground_speed =
            last_location.DistanceS(basic.location) / dt.count();
          basic.ground_speed_available.Update(basic.clock);
        } else
          basic.ground_speed_available.Clear();
      } else
        basic.ground_speed_available.Clear();

      AircraftState state;
      state.Reset();
      state.time = basic.time;
      state.location = basic.location;
      state.altitude = basic.GetAnyAltitude().value_or(0);
      state.ground_speed = basic.ground_speed_available
        ? basic.ground_speed
        : 0;

      FloatDuration dt{};
      if (last_time.IsDefined() && basic.time > last_time)
        dt = basic.time - last_time;
      else
        dt = FloatDuration{1};

      /* Clamp huge gaps so a logger pause does not explode the
         moving/stationary clocks. */
      if (dt > std::chrono::seconds{20})
        dt = std::chrono::seconds{20};

      const bool below_threshold = state.ground_speed <= takeoff_speed;

      if (!saw_takeoff && below_threshold)
        slow_before += dt;

      flying_computer.Compute(takeoff_speed, state, dt, flight);

      if (flight.flying && !saw_takeoff &&
          flight.takeoff_time.IsDefined()) {
        result.takeoff_utc = basic.GetDateTimeAt(flight.takeoff_time);
        saw_takeoff = result.takeoff_utc.IsPlausible();
      }

      if (!flight.flying && saw_takeoff && !saw_landing &&
          flight.landing_time.IsDefined()) {
        result.landing_utc = basic.GetDateTimeAt(flight.landing_time);
        saw_landing = result.landing_utc.IsPlausible();
        /* Count the stationary stretch that led to the detection. */
        if (saw_landing && basic.time > flight.landing_time)
          slow_after += basic.time - flight.landing_time;
      } else if (saw_landing && below_threshold)
        slow_after += dt;

      last_location = basic.location;
      last_time = basic.time;
    }
  } catch (...) {
    return false;
  }

  if (basic.time_available) {
    flying_computer.Finish(flight, basic.time);

    if (!flight.flying && saw_takeoff && !saw_landing &&
        flight.landing_time.IsDefined()) {
      result.landing_utc = basic.GetDateTimeAt(flight.landing_time);
      saw_landing = result.landing_utc.IsPlausible();
      if (saw_landing && basic.time > flight.landing_time)
        slow_after += basic.time - flight.landing_time;
    }
  }

  /* Logger stopped mid-flight: treat the last fix as landing. */
  if (saw_takeoff && !saw_landing && basic.date_time_utc.IsPlausible()) {
    result.landing_utc = basic.date_time_utc;
    saw_landing = true;
    forced_landing = true;
  }

  if (!(saw_takeoff && saw_landing && result.IsValid()))
    return false;

  result.started_too_late = slow_before < MIN_MARGIN;
  result.ended_too_early = forced_landing || slow_after < MIN_MARGIN;
  return true;
}
