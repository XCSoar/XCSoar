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

#include "FlightLogger.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "IO/TextWriter.hpp"

void
FlightLogger::Reset()
{
  last_time = fixed(0);
  seen_on_ground = seen_flying = false;
  start_time.Clear();
  landing_time.Clear();
}

void
FlightLogger::LogEvent(const BrokenDateTime &date_time, const char *type)
{
  assert(type != NULL);

  TextWriter writer(path.c_str(), true);
  if (!writer.IsOpen())
    /* Shall we log this error?  Not sure, because when this happens,
       usually the log file cannot be written either .. */
    return;

  /* XXX log pilot name, glider, airfield name */

  writer.FormatLine("%04u-%02u-%02uT%02u:%02u:%02u %s",
                    date_time.year, date_time.month, date_time.day,
                    date_time.hour, date_time.minute, date_time.second,
                    type);
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
  assert(!path.empty());

  if (basic.gps.replay || basic.gps.simulator)
    return;

  if (!basic.time_available || !basic.date_time_utc.IsDatePlausible())
    /* can't work without these */
    return;

  if (positive(last_time)) {
    fixed time_delta = basic.time - last_time;
    if (negative(time_delta) || time_delta > fixed(300))
      /* reset on time warp (positive or negative) */
      Reset();
    else if (time_delta < fixed(0.5))
      /* not enough time has passed since the last call: ignore this
         GPS fix, don't update last_time, just return */
      return;
    else
      TickInternal(basic, calculated);
  }

  last_time = basic.time;
}
