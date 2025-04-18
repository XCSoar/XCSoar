// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightLogger.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "LogFile.hpp"

void
FlightLogger::Reset()
{
  last_time = TimeStamp::Undefined();
  seen_on_ground = seen_flying = false;
  start_time.Clear();
  landing_time.Clear();
}

void
FlightLogger::LogEvent(const BrokenDateTime &date_time, const char *type)
try {
  assert(type != nullptr);

  FileOutputStream file(path, FileOutputStream::Mode::APPEND_OR_CREATE);
  BufferedOutputStream writer(file);

  /* XXX log pilot name, glider, airfield name */

  writer.Fmt("{:04}-{:02}-{:02}T{:02}:{:02}:{:02} {}\n",
             date_time.year, date_time.month, date_time.day,
             date_time.hour, date_time.minute, date_time.second,
             type);

  writer.Flush();
  file.Commit();
} catch (...) {
  LogError(std::current_exception());
}

void
FlightLogger::TickInternal(const MoreData &basic,
                           const DerivedInfo &calculated)
{
  const FlyingState &flight = calculated.flight;

  if (seen_on_ground && flight.flying) {
    /* store preliminary start time */
    start_time = basic.date_time_utc;

    if (!flight.on_ground) {
      /* start was confirmed (not on ground anymore): log it */
      seen_on_ground = false;

      LogEvent(start_time, "start");

      start_time.Clear();
    }
  }

  if (seen_flying && flight.on_ground) {
    /* store preliminary landing time */
    landing_time = basic.date_time_utc;

    if (!flight.flying) {
      /* landing was confirmed (not on ground anymore): log it */
      seen_flying = false;

      LogEvent(landing_time, "landing");

      landing_time.Clear();
    }
  }

  if (flight.flying && !flight.on_ground)
    seen_flying = true;

  if (!flight.flying && flight.on_ground)
    seen_on_ground = true;
}

void
FlightLogger::Tick(const MoreData &basic, const DerivedInfo &calculated)
{
  assert(path != nullptr);

  if (basic.gps.replay || basic.gps.simulator)
    return;

  if (!basic.time_available || !basic.date_time_utc.IsDatePlausible())
    /* can't work without these */
    return;

  if (last_time.IsDefined()) {
    auto time_delta = basic.time - last_time;
    if (time_delta.count() < 0 || time_delta > std::chrono::minutes{5})
      /* reset on time warp (positive or negative) */
      Reset();
    else if (time_delta < std::chrono::milliseconds{500})
      /* not enough time has passed since the last call: ignore this
         GPS fix, don't update last_time, just return */
      return;
    else
      TickInternal(basic, calculated);
  }

  last_time = basic.time;
}
